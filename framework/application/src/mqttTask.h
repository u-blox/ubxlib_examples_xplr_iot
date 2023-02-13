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
 * MQTT Task header
 *
 */
#ifndef _MQTT_TASK_H_
#define _MQTT_TASK_H_

/* ----------------------------------------------------------------
 * COMMON TASK FUNCTIONS
 * -------------------------------------------------------------- */
int32_t startMQTTTask(taskConfig_t *config);
void stopMQTTTask(void);

/* ----------------------------------------------------------------
 * TASK FUNCTIONS
 * -------------------------------------------------------------- */
int32_t sendMQTTMessage(const char *pTopicName, const char *pMessage, uMqttQos_t QoS, bool retain);
int32_t registerTopicCallBack(const char *topicName, uMqttQos_t maxQoS, void (*callbackFunction)(const char *, size_t));

/* ----------------------------------------------------------------
 * QUEUE MESSAGE TYPE DEFINITIONS
 * -------------------------------------------------------------- */
typedef enum {
    SEND_MQTT_MESSAGE,          // Sends a MQTT message
    SHUTDOWN_MQTT_TASK          // shuts down the 'task' by ending the mutex, queue and task.
} mqttMsgType_t;

/// @brief MQTT message to send
typedef struct SEND_MQTT_MESSAGE {
    char *pTopicName;
    char *pMessage;
    uMqttQos_t QoS;
    bool retain;
} sendMQTTMsg_t;

/// @brief Queue message structure for send any type of message to the MQTT application task
typedef struct MQTT_QUEUE_MESSAGE {
    mqttMsgType_t msgType;

    union {
        sendMQTTMsg_t message;
    } msg;
} mqttMsg_t;

#endif