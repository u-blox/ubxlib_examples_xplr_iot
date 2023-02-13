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
 * Signal Quality Task to monitor the signal quality of the network connection
 *
 */

#include "common.h"
#include "signalQualityTask.h"
#include "mqttTask.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define SIGNAL_QUALITY_DWELL_SECONDS 30
#define SIGNAL_QUALITY_TOPIC "SignalQuality"

#define SIGNAL_QUALITY_TASK_STACK_SIZE 1024
#define SIGNAL_QUALITY_TASK_PRIORITY 5
#define SIGNAL_QUALITY_QUEUE_STACK_SIZE 1024
#define SIGNAL_QUALITY_QUEUE_PRIORITY 5
#define SIGNAL_QUALITY_QUEUE_SIZE 5

/* ----------------------------------------------------------------
 * TASK COMMON VARIABLES
 * -------------------------------------------------------------- */
static bool exitTask = false;
static taskConfig_t *taskConfig;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static applicationStates_t tempAppStatus;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void dwellTask(void)
{
    // multiply by 10 as the TaskBlock is 100ms
    int32_t count = SIGNAL_QUALITY_DWELL_SECONDS * 10;
    for(int i = 0; !gExitApp && i < count; i++)
        uPortTaskBlock(100);
}

static void measureSignalQuality(void)
{
    char buffer[100];
    char topic[100];

    // create the topic name: pParameters is the serial number of this module.
    snprintf(topic, 100, "/%s/%s", (const char *)gSerialNumber, SIGNAL_QUALITY_TOPIC);
    writeLog("Signal Quality Topic Name: %s", topic);

    SET_APP_STATUS(START_SIGNAL_QUALITY);
    int32_t errorCode = uCellInfoRefreshRadioParameters(gDeviceHandle);
    if (errorCode == 0) {
        int32_t rsrp = uCellInfoGetRsrpDbm(gDeviceHandle);
        int32_t rsrq = uCellInfoGetRsrqDb(gDeviceHandle);

        snprintf(buffer, 100, "RSRP: %d, RSRQ: %d", rsrp, rsrq);
        if (rsrp != 0 && gIsNetworkUp) {
            gAppStatus = SEND_SIGNAL_QUALITY;
            sendMQTTMessage(topic, buffer, U_MQTT_QOS_AT_MOST_ONCE, false);
            writeLog(buffer);
        } else {
            writeLog("Can't publish Signal Quality results, network is not currently available");
        }
    } else {
        writeLog("Failed to read Radio Parameters %d", errorCode);
    }

    REVERT_APP_STATUS();
}

static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    signalQualityMsg_t *qMsg = (signalQualityMsg_t *) pParam;

    switch(qMsg->msgType) {
        case MEASURE_SIGNAL_QUALTY_NOW:
            measureSignalQuality();
            break;

        case SHUTDOWN_SIGNAL_QAULITY_TASK:
            stopSignalQualityTask();
            break;

        default:
            writeLog("Unknown message type: %d", qMsg->msgType);
            break;
    }
}

// Signal Quality task loop for reading the RSRP and RSRQ values
// and sending these values to the MQTT topic
static void taskLoop(void *pParameters)
{
    uPortMutexLock(TASK_MUTEX);
    while(!gExitApp  && !exitTask) {
        measureSignalQuality();
        dwellTask();
    }

    uPortMutexUnlock(TASK_MUTEX);
    writeLog("Signal Quality Task finished.");
}

static int32_t initQueue(taskConfig_t *config)
{
    int32_t eventQueueHandle = uPortEventQueueOpen(&queueHandler,
                    TASK_NAME,
                    sizeof(signalQualityMsg_t),
                    1024,
                    1,
                    1);

    if (eventQueueHandle < 0) {
        writeLog("Failed to create %s event queue %d", TASK_NAME, eventQueueHandle);
    }

    TASK_QUEUE = eventQueueHandle;

    return eventQueueHandle;
}


static int32_t initTask(taskConfig_t *config)
{
    int32_t errorCode = uPortTaskCreate(taskLoop,
                    TASK_NAME,
                    SIGNAL_QUALITY_TASK_STACK_SIZE,
                    (void *)gSerialNumber,
                    SIGNAL_QUALITY_TASK_PRIORITY,
                    &TASK_HANDLE);

    if (errorCode != 0) {
        writeLog("Failed to create %s Task (%d).", TASK_NAME, errorCode);
    }

    return errorCode;
}

static int32_t initMutex(taskConfig_t *config)
{
    int32_t errorCode = uPortMutexCreate(&TASK_MUTEX);
    if (errorCode != 0) {
        writeLog("Failed to create %s Mutex (%d).", TASK_NAME, errorCode);
    }

    return errorCode;
}

static void queueMeasureNow(void)
{
    signalQualityMsg_t msg;
    msg.msgType = MEASURE_SIGNAL_QUALTY_NOW;

    int32_t errorCode = uPortEventQueueSendIrq(TASK_QUEUE, &msg, sizeof(signalQualityMsgType_t));
    if (errorCode != 0) {
        writeLog("Failed sending Queue Message: %d", errorCode);
    }
}

static void controlCallback(const char *message, size_t msgSize)
{
    commandParams_t cmd;
    getParams((char *)message, &cmd);

    printf("Received %s command with %d params", cmd.command, cmd.count);

    if(strcmp(cmd.command, "MEASURE_NOW") == 0)
        queueMeasureNow();
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Starts the Signal Quality task
/// @param pParam Any parameter to be sent to the starting of the task
/// @return zero if successfull, a negative number otherwise
int32_t startSignalQualityTask(taskConfig_t *config)
{
    taskConfig = config;

    int32_t result = U_ERROR_COMMON_SUCCESS;

    writeLog("Starting the %s task...", TASK_NAME);
    CHECK_SUCCESS(initTask, config);
    CHECK_SUCCESS(initQueue, config);
    CHECK_SUCCESS(initMutex, config);

    subscribeToTopicAsync("SignalQualityControl", U_MQTT_QOS_AT_MOST_ONCE, controlCallback);

    return result;
}

void stopSignalQualityTask(void)
{
    exitTask = true;
    writeLog("Stop %s task requested...", taskConfig->name);
}