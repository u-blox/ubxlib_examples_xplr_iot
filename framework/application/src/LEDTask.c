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
 * LED task to light up and flash the three LEDs.
 *
 */

#include "common.h"
#include "LEDTask.h"
#include "leds.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define LED_TASK_STACK_SIZE 1024
#define LED_TASK_PRIORITY 5

/* ----------------------------------------------------------------
 * COMMON TASK VARIABLES
 * -------------------------------------------------------------- */
static bool exitTask = false;
static taskConfig_t *taskConfig;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * PUBLIC VARIABLES
 * -------------------------------------------------------------- */

// Application Status is mapped to this LED configuration array
#define MAX_LEDS 3
ledCfg_t ledStatus[MAX_STATUS][MAX_LEDS] = {
    {ledRedOff, ledGreenOff, ledBlueOff},           // MANUAL
    {ledRedFastPulse, ledGreenOff, ledBlueOff},     // INIT_DEVICE
    {ledRedOff, ledGreenOff, ledBlueBlink},         // REGISTERING
    {ledRedOff, ledGreenPulse, ledBlueOff},         // MQTT_CONNECTING
    {ledRedOff, ledGreenPulse, ledBlueOn},          // COPS_QUERY
    {ledRedFlash, ledGreenFlash, ledBlueOff},       // SEND_SIGNAL_QUALITY
    {ledRedOff, ledGreenOff, ledBlueFlash},         // REGISTRATION_UNKNOWN
    {ledRedOff, ledGreenOff, ledBlueOn},            // REGISTERED
    {ledRedOn, ledGreenOff, ledBlueOff},            // ERROR
    {ledRedOn, ledGreenOn, ledBlueOn},              // SHUTDOWN
    {ledRedOff, ledGreenOn, ledBlueOff},            // MQTT_CONNECTED
    {ledRedOff, ledGreenFlash, ledBlueOff},         // MQTT_DISCONNECTED
    {ledRedOn, ledGreenOn, ledBlueOff},             // START_SIGNAL_QUALITY
};

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */
static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    // does nothing, at the moment.
}

// LED task loop for turning on and off the LEDs according to the gAppStatus
static void taskloop(void *pParameters)
{
    U_PORT_MUTEX_LOCK(TASK_MUTEX);
    while(!exitTask) {
        int priorityLED = -1;

        // Calculate LED states and select priority LED
        for(int l = 0; l < MAX_LEDS; l++) {
            ledCfg_t *led = &ledStatus[gAppStatus][l];
            led->state = (led->timer < led->duty) != led->invert;
            led->timer = led->timer + ledTick_ms;
            if (led->timer >= led->period)
                led->timer = 0;
            if (priorityLED == -1 && led->priority && led->state)
                priorityLED = led->n;
        }

        // Set each LED state, priority LED overriding others.
        for(int l = 0; l < MAX_LEDS; l++) {
            ledCfg_t *led = &ledStatus[gAppStatus][l];
            if (priorityLED == -1)
                ledSet(led->n, led->state);
            else
                if (priorityLED == led->n)
                    ledSet(led->n, true);
                else
                    ledSet(led->n, false);
        }

        uPortTaskBlock(ledTick_ms);
    }

    // turn off the LEDs
    for(int l = 0; l < MAX_LEDS; l++) {
        ledSet(l, false);
    }

    U_PORT_MUTEX_UNLOCK(TASK_MUTEX);
    printf("LED Task finished.\n");
}

static int32_t initTask(taskConfig_t *config)
{
    int32_t errorCode = uPortTaskCreate(taskloop,
                    TASK_NAME,
                    LED_TASK_STACK_SIZE,
                    NULL,
                    LED_TASK_PRIORITY,
                    &TASK_HANDLE);

    if (errorCode != 0) {
        writeLog("Failed to create the %s Task (%d).",TASK_NAME, errorCode);
    }

    return errorCode;
}

static int32_t initQueue(taskConfig_t *config)
{
    int32_t eventQueueHandle = uPortEventQueueOpen(&queueHandler,
                    TASK_NAME,
                    sizeof(LEDConfigMsg_t),
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
        writeLog("Failed to create the %s Mutex (%d).", TASK_NAME, errorCode);
    }

    return errorCode;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

// Set individual Manual LED
void setManualLed(int n, ledCfg_t led)
{
    ledStatus[MANUAL][n] = led;
    gAppStatus = MANUAL;
}

// Set all three LEDS manually instead of using the ledStatus mapping above
void setManualLeds(ledCfg_t l1, ledCfg_t l2, ledCfg_t l3)
{
    setManualLed(0, l1);
    setManualLed(1, l2);
    setManualLed(2, l3);
}

// Copies the current LED statuses to the manual LED configuration
void copyLedsToManual()
{
    for(int i = 0; i < MAX_LEDS; i++)
        ledStatus[MANUAL][i] = ledStatus[gAppStatus][i];
}

// Copies current led statuses and updates one LED
void addLedToManual(int n, ledCfg_t led)
{
    copyLedsToManual();
    setManualLed(n, led);
}

/// @brief Starts the LED task(s)
/// @param pParam Any parameter to be sent to the starting of the task
/// @return zero if successfull, a negative number otherwise
int32_t startLEDTask(taskConfig_t *config)
{
    taskConfig = config;

    int32_t result = U_ERROR_COMMON_SUCCESS;

    writeLog("Starting the %s task...", TASK_NAME);
    CHECK_SUCCESS(initTask, config);
    CHECK_SUCCESS(initQueue, config);
    CHECK_SUCCESS(initMutex, config);

    return result;
}

/// @brief Stop the network manager and deregister from the cellular network
void stopLEDTask(void)
{
    exitTask = true;
    writeLog("Stop %s task requested...", taskConfig->name);
}
