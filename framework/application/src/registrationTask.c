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
 * Registration task to look after the network connection
 *
*/

#include "common.h"
#include "registrationTask.h"
#include "NTPClient.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define NETWORK_REGISTRATION_DWELL_SECONDS 30
#define BEGINNING_2023 1672531200

#define REG_TASK_STACK_SIZE 1024
#define REG_TASK_PRIORITY 5
#define REG_QUEUE_STACK_SIZE 1024
#define REG_QUEUE_PRIORITY 5
#define REG_QUEUE_SIZE 5

/* ----------------------------------------------------------------
 * TASK COMMON VARIABLES
 * -------------------------------------------------------------- */
static bool exitTask = false;
static taskConfig_t *taskConfig;

// This is here as it needs to be defined before the network cfg cell just below
static bool keepGoing(void *pParam)
{
    bool kg = !gExitApp && !exitTask;
    if (kg)
        printf("Still trying to register on a network...\n");
    else
        printf("Network registration cancelled\n");

    return kg;
}

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static const uNetworkCfgCell_t gNetworkCfg = {
    .type = U_NETWORK_TYPE_CELL,
    .pApn = APN,
    .pKeepGoingCallback = keepGoing,
    .timeoutSeconds = 240  // Connection timeout in seconds
};

static const uNetworkType_t gNetworkType = U_NETWORK_TYPE_CELL;

// This counts the number of times the network gets registered correctly
// At the start of the application this is zero until the first successful
// network registration. Until then this network manager will keep calling
// the UBXLIB uNetworkInterfaceUp() function
static volatile int32_t networkUpCounter = 0;

/* ----------------------------------------------------------------
 * PUBLIC VARIABLES
 * -------------------------------------------------------------- */

// This flag represents the network's registration status (false = unknown state)
bool gIsNetworkUp = false;

/// The unix network time, which is retrieved after first registration
extern int64_t unixNetworkTime;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Dwells for a number of seconds, checking the flags to
/// to see if it needs to exit early
static void dwellTask(void)
{
    // dwell at least 100ms at the start;
    uPortTaskBlock(100);

    // multiply by 10 as the TaskBlock is 100ms
    int32_t count = NETWORK_REGISTRATION_DWELL_SECONDS * 10;
    for(int i=0; !exitTask && !gExitApp && i < count; i++)
        uPortTaskBlock(100);
}

static void networkStatusCallback(uDeviceHandle_t devHandle,
                             uNetworkType_t netType,
                             bool isUp,
                             uNetworkStatus_t *pStatus,
                             void *pParameter)
{
    // count the number of times the network 'goes up'
    if (!gIsNetworkUp && isUp)
        networkUpCounter++;

    writeLog("Network Status: %s", isUp ? "Registered" : "Unknown");
    gIsNetworkUp = isUp;

    if (!isUp)
        gAppStatus = REGISTRATION_UNKNOWN;
}

static void getNetworkOrNTPTime(void)
{
    // try and get the network time from the cellular network
    int64_t time = uCellInfoGetTimeUtc(gDeviceHandle);

    // if the network time is less than 2023, assume it is wrong
    // and get the time from NTP service
    if (time < BEGINNING_2023) {
        time = getNTPTime();
    }

    // if time is positive, it should be now a valid time
    if (time > 0) {
        // remove the current local tick time from this
        // to obtain a rough "boot" time value.
        unixNetworkTime = time - (uPortGetTickTimeMs() / 1000);
    }
}

static int32_t startNetworkRegistration(void)
{
    gAppStatus = REGISTERING;
    writeLog("Bringing up the cellular network...");
    int32_t errorCode = uNetworkInterfaceUp(gDeviceHandle, gNetworkType, &gNetworkCfg);
    if (errorCode != 0) {
        writeLog("Failed to bring up the cellular network: %d", errorCode);
        return errorCode;
    }

    errorCode = uNetworkSetStatusCallback(gDeviceHandle, gNetworkType, networkStatusCallback, NULL);
    if (errorCode != 0) {
        writeLog("Failed to set the network status callback: %d", errorCode);
        return errorCode;
    }

    gIsNetworkUp = true;
    networkUpCounter=1;

    writeLog("Connected to Cellular Network");
    return 0;
}

static int32_t deRegisterFromNetwork(void)
{
    gAppStatus = REGISTERING;
    writeLog("Deregistering from the network...");
    int32_t errorCode = uNetworkInterfaceDown(gDeviceHandle, gNetworkType);
    if (errorCode != 0) {
        writeLog("Failed to de-register from the cellular network: %d", errorCode);
    } else {
        writeLog("Deregistered from cellular network");
    }

    return errorCode;
}

static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    // does nothing, at the moment.
}

// Network registration manager task loop
static void taskLoop(void *pParameters)
{
    uPortMutexLock(TASK_MUTEX);
    while(!exitTask) {
        // if the application is exiting, we don't need to try and
        // manage the network connection... just wait.
        if (!gExitApp) {
            // If we've never seen the network has been up before, start the reg process...
            if (networkUpCounter==0) {
                if (startNetworkRegistration() == 0) {
                    getNetworkOrNTPTime();
                }
            }

            // logging tick about the network reg status
            if (gIsNetworkUp) {
                writeLog("Network is up and running");
            } else {
                gAppStatus = REGISTRATION_UNKNOWN;
                writeLog("Unknown network registration state");
                // What can we do here? not a lot.
            }
        }

        dwellTask();
    }

    // we've been asked to exit the Network Manager, so go through
    // the de-registration process before finishing
    deRegisterFromNetwork();

    uPortMutexUnlock(TASK_MUTEX);
    writeLog("Network Registration Task finished.");
}

static int32_t initTask(taskConfig_t *config)
{
    int32_t errorCode = uPortTaskCreate(taskLoop,
                    TASK_NAME,
                    REG_TASK_STACK_SIZE,
                    NULL,
                    REG_TASK_PRIORITY,
                    &TASK_HANDLE);

    if (errorCode != 0) {
        writeLog("Failed to create %s task (%d).", TASK_NAME, errorCode);
    }

    return errorCode;
}

static int32_t initQueue(taskConfig_t *config)
{
    int32_t eventQueueHandle = uPortEventQueueOpen(&queueHandler,
                    TASK_NAME,
                    sizeof(registrationMsg_t),
                    1024,
                    1,
                    1);

    if (eventQueueHandle < 0) {
        writeLog("Failed to create %s event queue %d", TASK_NAME, eventQueueHandle);
    }

    TASK_QUEUE = eventQueueHandle;

    return eventQueueHandle;
}


static int32_t initMutex(taskConfig_t *config)
{
    int32_t errorCode = uPortMutexCreate(&TASK_MUTEX);
    if (errorCode != 0) {
        writeLog("Failed to create %s Mutex (%d).", TASK_NAME, errorCode);
    }

    return errorCode;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Starts the registration task
/// @param pParam Any parameter to be sent to the starting of the task
/// @return zero if successfull, a negative number otherwise
int32_t startNetworkRegistrationTask(taskConfig_t *config)
{
    taskConfig = config;

    int32_t result = U_ERROR_COMMON_SUCCESS;

    writeLog("Starting the %s task...", TASK_NAME);
    CHECK_SUCCESS(initTask, config);
    CHECK_SUCCESS(initQueue, config);
    CHECK_SUCCESS(initMutex, config);

    return result;
}

void stopNetworkRegistrationTask(void)
{
    exitTask = true;
    writeLog("Stop %s task requested...", taskConfig->name);
}