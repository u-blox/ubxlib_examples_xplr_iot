/*
 * Copyright 2022 u-blox
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 *
 * Example showing how to use the buttons and leds in the XPLR-IOT-1
 *
 */

#include "common.h"
#include "ext_fs.h"
#include "leds.h"
#include "buttons.h"

#include "LEDTask.h"
#include "mqttTask.h"
#include "registrationTask.h"
#include "signalQualityTask.h"
#include "cellScanTask.h"
#include "exampleTask.h"

typedef enum {
    NO_BUTTON=-1,
    BUTTON_1=0,
    BUTTON_2=1
} buttonNumber_t;

static bool buttonCommandEnabled = false;
static buttonNumber_t pressedButton = NO_BUTTON;

#define STARTUP_DELAY 250        // 250 * 20ms => 5 seconds
#define LOG_FILENAME "log.csv"

applicationStates_t gAppStatus = MANUAL;

// serial number of the module
char gSerialNumber[U_SECURITY_SERIAL_NUMBER_MAX_LENGTH_BYTES];

// deviceHandle is not static as this is shared between other modules.
uDeviceHandle_t gDeviceHandle;

// This flag is set to true when the application should close tasks and log files.
// This flag is set to true when Button #1 is pressed.
bool gExitApp = false;

static uDeviceType_t deviceType = U_DEVICE_TYPE_CELL;
static uDeviceCfg_t deviceCfg;

// enum of task types - make sure this is in the same
// order as the taskRunner_t tasks initialiser below!
typedef enum {
    NETWORK_REG_TASK = 0,
    CELL_SCAN_TASK = 1,
    MQTT_TASK = 2,
    SIGNAL_QUALITY_TASK = 3,
    LED_TASK = 4,
    EXAMPLE_TASK = 5,
    MAX_TASKS
} taskTypeId_t;

// These task runners define the application.
// Here we specify what tasks are to run, and what configuration they are to use
taskRunner_t tasks[] = {
    {startNetworkRegistrationTask, stopNetworkRegistrationTask, {"Registration", NULL, {NULL, NULL, 0}}},
    {startCellScanTask, stopCellScanTask, {"Cell Scan", NULL, {NULL, NULL, 0}}},
    {startMQTTTask, stopMQTTTask, {"MQTT", NULL, {NULL, NULL, 0}}},
    {startSignalQualityTask, stopSignalQualityTask, {"Signal Quality", NULL, {NULL, NULL, 0}}},
    {startLEDTask, stopLEDTask, {"LED", NULL, {NULL, NULL, 0}}},
    {startExampleTask, stopExampleTask, {"Example", NULL, {NULL, NULL, 0}}}
};

#define TASK_CONFIG(task) tasks[task].config
#define IS_TASK_RUNNING(task) isTaskRunning(TASK_CONFIG(task).handles.mutexHandle)

/// @brief Handler for the buttons. On boot, Button 1: Display log, 2: Delete log.
static void button_pressed(int buttonNo, uint32_t holdTime)
{
    if (gExitApp)
        return;

    if (!holdTime) {
        pressedButton = buttonNo;
    } else {
        pressedButton = NO_BUTTON;
        if (!buttonCommandEnabled)
            return;

        switch (buttonNo)
        {
        case BUTTON_1:
            writeLog("Exit button pressed, closing down...");
            gExitApp = true;
            break;
        case BUTTON_2:
            startNetworkScan(gSerialNumber);
            break;
        default:
            break;
        }
    }
}

/// @brief Function to check for a held button at start up.
/// @return The button that was held at start up.
static buttonNumber_t checkStartButton(void) {
    ledSet(2, true);
    // wait for 5 seconds, or if a button is pressed
    for(int i = 0; pressedButton == NO_BUTTON && i < STARTUP_DELAY; i++)
        uPortTaskBlock(20);
    ledSet(2, false);

    buttonNumber_t heldButton = pressedButton;
    if (heldButton == NO_BUTTON)
        return NO_BUTTON;

    // turn on the correct LED for the operation
    int led = 0;  // Delete log file = red
    if (heldButton == BUTTON_1)
        led = 1; // Display log file = green
    ledSet(led, true);

    // wait until the button is released
    while(pressedButton != NO_BUTTON)
        uPortTaskBlock(20);

    return heldButton;
}

/// @brief Reads the serial number from the module
/// @return The length of the serial number, or negative on error
static int32_t getSerialNumber(void)
{
    int32_t len = uSecurityGetSerialNumber(gDeviceHandle, gSerialNumber);
    if (len > 0) {
        if (gSerialNumber[0] == '"') {
            // Remove quotes
            memmove(gSerialNumber, gSerialNumber + 1, len);
            gSerialNumber[len - 2] = 0;
        }

        writeLog("Cellular Module Serial Number: %s", gSerialNumber);
    }

    return len;
}

/// @brief Initiate the UBXLIX API
static int32_t initDevice(void) {
    int32_t errorCode;

    // turn off the UBXLIB printf() logging
    //uPortLogOff();

    writeLog("Initiating the UBXLIB Device API...");
    errorCode = uDeviceInit();
    if (errorCode != 0) {
        writeLog("* Failed to initiate the UBXLIB device API: %d", errorCode);
        return errorCode;
    }

    uDeviceGetDefaults(deviceType, &deviceCfg);

    writeLog("Opening/Turning on the cellular module...");
    errorCode = uDeviceOpen(&deviceCfg, &gDeviceHandle);
    if (errorCode != 0) {
        writeLog("* Failed to turn on the cellular module: %d", errorCode);
        return errorCode;
    }

    // get serial number
    errorCode = getSerialNumber();
    if (errorCode < 0) {
        writeLog("* Failed to get the serial number of the module: %d", errorCode);
        return errorCode;
    }

    return 0;
}

/// @brief Checks the "isTaskRunningxxxx()" functions and returns when the tasks have all stopped.
static void waitForTasksToStop(taskConfig_t *taskConfigs, size_t numTasks)
{
    bool stillWaiting;

    printf("Waiting for the tasks to finish...\n");
    do
    {
        stillWaiting = false;

        for(int i=0; i<numTasks; i++) {
            if (isTaskRunning(taskConfigs[i].handles.mutexHandle)) {
                printf("Still waiting for %s task to finish...\n", taskConfigs[i].name);
                stillWaiting = true;
            }
        }

        uPortTaskBlock(2000);
    } while (stillWaiting);

    printf("All tasks are now finished...\n");
}

/// @brief Blocking function while waiting for the task to finish
/// @param taskConfig The task Configuration to wait for
static void waitForTaskToStop(taskConfig_t taskConfig)
{
    while(isTaskRunning(taskConfig.handles.mutexHandle)) {
        writeLog("Waiting for %s task to stop...", taskConfig.name);
        uPortTaskBlock(2000);
    }

    writeLog("%s task has stopped.", taskConfig.name);
}

/// @brief Sets the application status, waits for the tasks and closes the log
/// @param appState The application status to set for the shutdown
static void finalise(applicationStates_t appState)
{
    gAppStatus = appState;
    gExitApp = true;

    // Tell the common tasks to finish first
    taskConfig_t checkTasks[] = {TASK_CONFIG(SIGNAL_QUALITY_TASK),
                                TASK_CONFIG(MQTT_TASK),
                                TASK_CONFIG(CELL_SCAN_TASK),
                                TASK_CONFIG(EXAMPLE_TASK)};

    waitForTasksToStop(checkTasks, ARRAYSIZE(checkTasks));

    // now stop the network registration task
    stopNetworkRegistrationTask();
    waitForTaskToStop(TASK_CONFIG(NETWORK_REG_TASK));

    // nothing left to do, so close the log
    closeLog();

    // finally stop the LED task
    tasks[LED_TASK].stopFunc();
    waitForTaskToStop(TASK_CONFIG(LED_TASK));

    printf("Application has finished.\n");
}

static int32_t runTasks(taskRunner_t *pTaskRunner, size_t numRunners)
{
    int32_t errorCode;

    for(int i=0; i<numRunners; i++) {
        if (IS_TASK_RUNNING(i))
            continue;

        errorCode = pTaskRunner->startFunc(&pTaskRunner->config);
        if (errorCode != 0) {
            writeLog("* Failed to start the %s task (%d) - not running application!", pTaskRunner->config.name, errorCode);
            return errorCode;
        }

        writeLog("Started %s task", pTaskRunner->config.name);
        pTaskRunner++;
    }

    return 0;
}

static bool initFramework(void)
{
    uPortInit();
    if (!buttonsInit(button_pressed)) {
        printf("* Failed to initiate buttons - not running application!\n");
        return false;
    }
    if (!ledsInit()) {
        printf("* Failed to initiate leds - not running application!\n");
        return false;
    }
    if (!extFsInit()) {
        printf("* Failed to mounth File System - not running application!\n");
        return false;
    }

    // User has chance to hold down a button to delete or display the log
    buttonNumber_t button = checkStartButton();

    // deleting the log file is performed now before the start of the application
    if (button == BUTTON_2) {
        printf("Deleting log file...\n");
        deleteFile(LOG_FILENAME);
    }

    startLogging(LOG_FILENAME);

    // displaying the log file ends the application
    if (button == BUTTON_1) {
        displayLogFile();
        ledSet(1, false);
        printf("Application finished");
        return false;
    }

    // now allow the buttons to run their commands
    buttonCommandEnabled = true;

    return true;
}

/// @brief Sends a task a message via its event queue
/// @param taskId The TaskId (based on the taskTypeId_t)
/// @param message pointer to the message to send
/// @param msgSize the size of the message to send
/// @return 0 on success, negative on failure
int32_t sendAppTaskMessage(int32_t taskId, const void *pMessage, size_t msgSize)
{
    if (taskId < 0 || taskId >= MAX_TASKS) {
        writeLog("Send App Task [%d] Message Error: Invalid Task Id", taskId);
        return -1;
    }

    int32_t errorCode = uPortEventQueueSendIrq(TASK_CONFIG(taskId).handles.eventQueueHandle, pMessage, msgSize);
    if (errorCode < 0) {
        writeLog("Send App Task [%d] Message Error: Can't send event queue message: %d", taskId, errorCode);
    }

    return errorCode;
}

static void controlMessageHandler(const void *message, size_t msgSize) {
    printf("Received control message\n");
}

static void controlTopicSubscription(void *pParam)
{
    // wait until the MQTT Task is up and running
    while(isTaskRunning(TASK_CONFIG(MQTT_TASK).handles.mutexHandle)) {
        uPortTaskBlock(500);
    }

    char controlTopicName[50];
    snprintf(controlTopicName, 50, "/%s/Control", gSerialNumber);

    // the MQTT task is running, but we might not be connected yet so this can fail
    // U_ERROR_COMMON_NOT_INITIALISED is the error if the MQTT client isn't connected yet.
    int32_t errorCode = U_ERROR_COMMON_NOT_INITIALISED;
    while(errorCode == U_ERROR_COMMON_NOT_INITIALISED) {
        errorCode = registerTopicCallBack(controlTopicName, U_MQTT_QOS_AT_MOST_ONCE, &controlMessageHandler);
        if(errorCode == U_ERROR_COMMON_NOT_INITIALISED)
            uPortTaskBlock(5000);
    }
}

/// @brief Main entry to the application.
/// If Button 1 is held, the log file is displayed.
/// If Button 2 is held, the log file is deleted.
/// In both cases, the application starts as normal after.
void main(void)
{
    int32_t errCode;

    // initialise our LEDs and start up button commmands
    if (!initFramework())
        return;

    /* ************************************************************************
     * Start the LED task first, as this is our only indication anything is running.
     * ************************************************************************/
    writeLog("Starting LED Task...");
    errCode = startLEDTask(&TASK_CONFIG(LED_TASK));
    if (errCode != 0) {
        writeLog("* Failed to start LED task - not running application!");
        return finalise(ERROR);
    }

    /* ************************************************************************
     * Initialisation section for Cellular Module
     * ************************************************************************/

    // initialise the cellular module
    gAppStatus = INIT_DEVICE;
    errCode = initDevice();
    if (errCode != 0) {
        writeLog("* Failed to initialise the cellular module - not running application!");
        return finalise(ERROR);
    }

    /* ************************************************************************
     * Start the application tasks (already running won't be restarted)
     * ************************************************************************ */
    if (runTasks(tasks, ARRAYSIZE(tasks)) != 0) {
        return finalise(ERROR);
    }

    uPortTaskHandle_t handle;
    uPortTaskCreate(controlTopicSubscription, NULL, 1024, NULL, 1, &handle);

    /* ************************************************************************
     * Application loop and finalisation on exit stage
     * ************************************************************************ */

    while(!gExitApp) {
        printf("\n*************\n");
        printf("** RUNNING **\n");
        printf("*************\n");
        uPortTaskBlock(2000);
    }

    writeLog("Application closing down...");
    finalise(SHUTDOWN);
}
