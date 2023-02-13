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
 * Cell Scan Task to run the +COPS=? Query and publish the results
 *
 */

#include "common.h"
#include "cellScanTask.h"
#include "mqttTask.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define NETWORK_SCAN_TOPIC "NetworkScan"

#define CELL_SCAN_TASK_STACK_SIZE (3*1024)
#define CELL_SCAN_TASK_PRIORITY 5

#define CELL_SCAN_QUEUE_STACK_SIZE (1*1024)
#define CELL_SCAN_QUEUE_PRIORITY 5
#define CELL_SCAN_QUEUE_SIZE 2

/* ----------------------------------------------------------------
 * COMMON TASK VARIABLES
 * -------------------------------------------------------------- */
static bool exitTask = false;
static taskConfig_t *taskConfig;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static applicationStates_t tempAppStatus;

static bool stopCellScan = false;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static bool keepGoing(void *pParam)
{
    bool kg = !gExitApp && !exitTask && !stopCellScan;
    if (kg) {
        gAppStatus = COPS_QUERY;
        printf("Still scanning for networks...\n");
    } else {
        printf("Scanning for networks cancelled\n");
    }

    return kg;
}

static void doCellScan(void *pTopicName)
{
    int32_t found = 0;
    int32_t count = 0;
    char internalBuffer[64];
    char payload[200];
    char mccMnc[U_CELL_NET_MCC_MNC_LENGTH_BYTES];

    U_PORT_MUTEX_LOCK(TASK_MUTEX);
    SET_APP_STATUS(COPS_QUERY);

    // create the topic name
    char topic[100];
    snprintf(topic, 100, "/%s/%s", (const char *)pTopicName, NETWORK_SCAN_TOPIC);
    writeLog("Cell Scan Topic: %s", topic);

    writeLog("Scanning for networks...");
    for (count = uCellNetScanGetFirst(gDeviceHandle, internalBuffer,
                                            sizeof(internalBuffer), mccMnc, NULL,
                                            keepGoing);
            count >= 0;
            count = uCellNetScanGetNext(gDeviceHandle, internalBuffer, sizeof(internalBuffer), mccMnc, NULL)) {

        found++;
        snprintf(payload, sizeof(payload), "Cell Scan Result: found '%s', MCC/MNC: %s", internalBuffer, mccMnc);
        writeLog(payload);
        sendMQTTMessage(topic, payload, U_MQTT_QOS_AT_MOST_ONCE, false);
    }

    if (!gExitApp) {
        if(count < 0) {
            snprintf(payload, sizeof(payload), "Cell Scan Result: Error %d", count);
        } else {
            if (found == 0) {
                snprintf(payload, sizeof(payload), "Cell Scan Result: No network operators found.");
            } else {
                snprintf(payload, sizeof(payload), "Cell Scan Result: %d network(s) found in total.", found);
            }
        }
    } else {
        snprintf(payload, sizeof(payload), "Cell Scan Result: Cancelled.");
    }

    writeLog(payload);
    sendMQTTMessage(topic, payload, U_MQTT_QOS_AT_MOST_ONCE, false);

    // reset the stop cell scan indicator
    stopCellScan = false;

    REVERT_APP_STATUS();
    U_PORT_MUTEX_UNLOCK(TASK_MUTEX);
}

void startCellScan(const char *pTopicName)
{
    int32_t errorCode = uPortTaskCreate(doCellScan,
                NULL,
                CELL_SCAN_TASK_STACK_SIZE,
                (void *)pTopicName,
                CELL_SCAN_TASK_PRIORITY,
                &TASK_HANDLE);

    if (errorCode < 0) {
        writeLog("Failed to start cell scan task: %d", errorCode);
    }
}

static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    cellScanMsg_t *qMsg = (cellScanMsg_t *) pParam;
    printf("cell scan message: %d", qMsg->msgType);

    switch(qMsg->msgType) {
        case START_CELL_SCAN:
            startCellScan(qMsg->msg.topicName);
            break;

        case STOP_CELL_SCAN:
            stopCellScan = true;
            break;

        case SHUTDOWN_CELL_SCAN_TASK:
            exitTask = true;
            break;

        default:
            writeLog("Unknown message type: %d", qMsg->msgType);
            break;
    }
}

static int32_t initTask(taskConfig_t *config)
{
    int32_t errorCode = U_ERROR_COMMON_SUCCESS;

    if (errorCode != 0){
        writeLog("Failed to initialise the %s task (%d). ", TASK_NAME, errorCode);
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

static int32_t initQueue(taskConfig_t *config)
{
    int32_t eventQueueHandle = uPortEventQueueOpen(&queueHandler,
                    TASK_NAME,
                    sizeof(cellScanMsg_t),
                    CELL_SCAN_QUEUE_STACK_SIZE,
                    CELL_SCAN_QUEUE_PRIORITY,
                    CELL_SCAN_QUEUE_SIZE);

    if (eventQueueHandle < 0) {
        writeLog("Failed to create %s event queue %d", TASK_NAME, eventQueueHandle);
    }

    TASK_QUEUE = eventQueueHandle;

    return eventQueueHandle;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Places a Start Network Scan message on the queue
/// @param pSerialNumber the serial number of the device, for the MQTT topic name
/// @return zero if successfull, a negative value otherwise
int32_t startNetworkScan(const char *pSerialNumber)
{
    int32_t errorCode = (int32_t) U_ERROR_COMMON_SUCCESS;

    // if the mutex or queue handle is not valid, don't queue a message
    if (TASK_MUTEX == NULL || TASK_QUEUE < 0) {
        writeLog("Cell Scan Event Queue is not running, not queueing Network Scan command");
        return -1;
    }

    cellScanMsg_t msg;
    if (isTaskRunning(TASK_MUTEX)) {
        writeLog("Cell Scan is already in progress, cancelling...");
        msg.msgType = STOP_CELL_SCAN;
    } else {
        writeLog("Starting cell scan...");
        msg.msgType = START_CELL_SCAN;
        msg.msg.topicName = pSerialNumber;
    }

    errorCode = uPortEventQueueSendIrq(TASK_QUEUE, &msg, sizeof(cellScanMsg_t));
    if (errorCode != 0) {
        writeLog("Failed to queue a network scan event: %d", errorCode);
    }

    return errorCode;
}

/// @brief Starts the network scanning task(s)
/// @param pParam Any parameter to be sent to the starting of the task
/// @return zero if successfull, a negative number otherwise
int32_t startCellScanTask(taskConfig_t *config)
{
    taskConfig = config;

    int32_t result = U_ERROR_COMMON_SUCCESS;

    writeLog("Starting the %s task...", TASK_NAME);
    CHECK_SUCCESS(initTask, config);
    CHECK_SUCCESS(initQueue, config);
    CHECK_SUCCESS(initMutex, config);

    return result;
}

void stopCellScanTask(void)
{
    exitTask = true;
    writeLog("Stop %s task requested...", taskConfig->name);
}