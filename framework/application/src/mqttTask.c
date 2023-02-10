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
 * MQTT Task to connect to the broker and to keep the connection
 *
 */

#include "common.h"
#include "mqttTask.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define MQTT_MANAGE_DWELL_SECONDS 30

#define MQTT_TASK_STACK_SIZE 1024
#define MQTT_TASK_PRIORITY 5
#define MQTT_QUEUE_STACK_SIZE 1024
#define MQTT_QUEUE_PRIORITY 5
#define MQTT_QUEUE_SIZE 5

#define COPYTO(msg, x) ((msg.x = uStrDup(x))==NULL) ? true : failed

/* ----------------------------------------------------------------
 * COMMON TASK VARIABLES
 * -------------------------------------------------------------- */
static bool exitTask = false;
static taskConfig_t *taskConfig;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */
static uMqttClientContext_t *pContext = NULL;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/// Frees the malloced memory within the send message structure
static void freeMsg(sendMQTTMsg_t msg)
{
    free(msg.pTopicName);
    free(msg.pMessage);
}

static void mqttSendMessage(sendMQTTMsg_t msg)
{
    int32_t errorCode = 0;

    if (pContext == NULL || !uMqttClientIsConnected(pContext)) {
        writeLog("MQTT Client is not online, not publishing message");
        return;
    }

    gAppStatus = MQTT_CONNECTED;
    errorCode = uMqttClientPublish(pContext, msg.pTopicName, msg.pMessage,
                                                   strlen(msg.pMessage),
                                                   msg.QoS,
                                                   msg.retain);

    freeMsg(msg);

    if (errorCode == 0)
        writeLog("Published MQTT message");
    else
        writeLog("Failed to publish MQTT message");
}

static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    mqttMsg_t *qMsg = (mqttMsg_t *) pParam;

    switch(qMsg->msgType) {
        case SEND_MQTT_MESSAGE:
            mqttSendMessage(qMsg->msg.message);
            break;

        case SHUTDOWN_MQTT_TASK:
            stopMQTTTask();
            break;

        default:
            writeLog("Unknown message type: %d", qMsg->msgType);
            break;
    }
}

static void disconnectCallback(int32_t errorCode, void *param)
{
    if (errorCode != 0) {
        gAppStatus = MQTT_DISCONNECTED;
        writeLog("MQTT Disconnect callback with Error Code: %d", errorCode);
    }
}

static void downlinkMessageCallback(int32_t errorCode, void *param)
{
    writeLog("Received downlink message");
}

static int32_t connectBroker(void)
{
    gAppStatus = MQTT_CONNECTING;
    writeLog("Connecting to MQTT Broker...");
    uMqttClientConnection_t connection = U_MQTT_CLIENT_CONNECTION_DEFAULT;
    connection.pBrokerNameStr = BROKER_NAME;
    connection.pUserNameStr = USERNAME;
    connection.pPasswordStr = PASSWORD;
    connection.inactivityTimeoutSeconds = 0; // zero = no timeout
    connection.keepAlive = false;

    int32_t errorCode = uMqttClientConnect(pContext, &connection);
    if (errorCode != 0) {
        writeLog("Failed to connect to the MQTT Broker: %d", errorCode);
        return errorCode;
    }

    gAppStatus = MQTT_CONNECTED;
    errorCode = uMqttClientSetDisconnectCallback(pContext, disconnectCallback, NULL);
    if (errorCode != 0) {
        writeLog("Failed to set MQTT Disconnect callback: %d", errorCode);
        return errorCode;
    }

    errorCode = uMqttClientSubscribe(pContext, "", U_MQTT_QOS_AT_MOST_ONCE);
    if (errorCode != 0) {
        writeLog("Failed to subsribe to the XPLR Application Control topic: %d", errorCode);
        return errorCode;
    }

    errorCode = uMqttClientSetMessageCallback(pContext, downlinkMessageCallback, NULL);
    if (errorCode != 0) {
        writeLog("Failed to set MQTT downlink message callback: %d", errorCode);
        return errorCode;
    }


    writeLog("Connected to MQTT Broker");
    return 0;
}

static int32_t disconnectBroker(void)
{
    writeLog("Disconnecting from MQTT broker...");
    int32_t errorCode = uMqttClientDisconnect(pContext);
    if (errorCode != 0) {
        writeLog("Failed to disconnect from MQTT Broker: %d", errorCode);
    } else {
        writeLog("Disconnected from MQTT Broker");
    }

    return errorCode;
}

static void dwellTask(void)
{
    // multiply by 10 as the TaskBlock is 100ms
    int32_t count = MQTT_MANAGE_DWELL_SECONDS * 10;
    for(int i = 0; !gExitApp && i < count; i++)
        uPortTaskBlock(100);
}

/// @brief Task loop for the MQTT management
/// @param pParameters
static void taskLoop(void *pParameters)
{
    U_PORT_MUTEX_LOCK(TASK_MUTEX);
    while(!gExitApp && !exitTask)
    {
        if (!uMqttClientIsConnected(pContext)) {
            gAppStatus = MQTT_DISCONNECTED;
            if (gIsNetworkUp) {
                writeLog("MQTT client disconnected, trying to connect...");
                connectBroker();
            } else {
                writeLog("Can't connect to MQTT Broker, module is not registered at the moment.");
            }
        }

        dwellTask();
    }

    // Application exiting, so disconnect from MQTT Broker...
    disconnectBroker();

    U_PORT_MUTEX_UNLOCK(TASK_MUTEX);
    writeLog("MQTT Manager Task finished.");
}

static int32_t initTask(taskConfig_t *config)
{
    pContext = pUMqttClientOpen(gDeviceHandle, NULL);
    if (pContext == NULL) {
        writeLog("Failed to open the MQTT client");
        return -1;
    }

    int32_t errorCode = uPortTaskCreate(taskLoop,
                    TASK_NAME,
                    MQTT_TASK_STACK_SIZE,
                    NULL,
                    MQTT_TASK_PRIORITY,
                    &TASK_HANDLE);

    if (errorCode != 0) {
        writeLog("Failed to start the %s Task (%d).", TASK_NAME, errorCode);
    }

    return errorCode;
}

static int32_t initQueue(taskConfig_t *config)
{
    int32_t eventQueueHandle = uPortEventQueueOpen(&queueHandler,
                    TASK_NAME,
                    sizeof(mqttMsg_t),
                    MQTT_QUEUE_STACK_SIZE,
                    MQTT_QUEUE_PRIORITY,
                    MQTT_QUEUE_SIZE);

    if (eventQueueHandle < 0) {
        writeLog("Failed to create MQTT event queue %d.", eventQueueHandle);
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
/// @brief Puts a message on to the MQTT publish queue
/// @param pTopicName a pointer to the topic name which is copied
/// @param pMessage a pointer to the message text which is copied
/// @param QoS the Quality of Service value for this message
/// @param retain If the message should be retained
/// @return 0 if successfully queued on the event queue
int32_t sendMQTTMessage(const char *pTopicName, const char *pMessage, uMqttQos_t QoS, bool retain)
{
    // if the event queue handle is not valid, don't send the message
    if (TASK_QUEUE < 0) {
        writeLog("MQTT Event Queue handle is not valid, not publishing MQTT message");
        return -1;
    }

    mqttMsg_t qMsg;
    qMsg.msgType = SEND_MQTT_MESSAGE;

    bool failed = false;
    failed = COPYTO(qMsg.msg.message, pTopicName);
    failed = COPYTO(qMsg.msg.message, pMessage);

    if (failed) {
        freeMsg(qMsg.msg.message);
        writeLog("Failed to allocate memory for MQTT message.");
        return U_ERROR_COMMON_NO_MEMORY;
    }

    qMsg.msg.message.QoS = QoS;
    qMsg.msg.message.retain = retain;

    return uPortEventQueueSend(TASK_QUEUE, &qMsg, sizeof(mqttMsg_t));
}

/// @brief Starts the MQTT task
/// @param pParam Any parameter to be sent to the starting of the task
/// @return zero if successfull, a negative number otherwise
int32_t startMQTTTask(taskConfig_t *config)
{
    taskConfig = config;

    int32_t result = U_ERROR_COMMON_SUCCESS;

    writeLog("Starting the %s task...", TASK_NAME);
    CHECK_SUCCESS(initTask, config);
    CHECK_SUCCESS(initQueue, config);
    CHECK_SUCCESS(initMutex, config);

    sendAppTaskMessage(-1, NULL, 0);

    return result;
}

void stopMQTTTask(void)
{
    exitTask = true;
    writeLog("Stop %s task requested...", taskConfig->name);
}
