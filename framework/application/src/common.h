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
 * Application header
 *
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stdbool.h"
#include "stdint.h"

#include "ubxlib.h"
#include "config.h"
#include "log.h"

/* ----------------------------------------------------------------
 * MACORS for common task usage/access
 * -------------------------------------------------------------- */
#define SET_APP_STATUS(x) tempAppStatus = gAppStatus; gAppStatus = x
#define REVERT_APP_STATUS(x) gAppStatus = tempAppStatus

#define CHECK_SUCCESS(x, config) result = x(config); if (result < 0) return result
#define TASK_MUTEX taskConfig->handles.mutexHandle
#define TASK_HANDLE taskConfig->handles.taskHandle
#define TASK_QUEUE taskConfig->handles.eventQueueHandle
#define TASK_NAME taskConfig->name

#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))

/* ----------------------------------------------------------------
 * PUBLIC TYPE DEFINITIONS
 * -------------------------------------------------------------- */
// Default set of application statuses
typedef enum {
    MANUAL,
    INIT_DEVICE,
    REGISTERING,
    MQTT_CONNECTING,
    COPS_QUERY,
    SEND_SIGNAL_QUALITY,
    REGISTRATION_UNKNOWN,
    REGISTERED,
    ERROR,
    SHUTDOWN,
    MQTT_CONNECTED,
    MQTT_DISCONNECTED,
    START_SIGNAL_QUALITY,
    MAX_STATUS
} applicationStates_t;

typedef struct TaskHandles {
    uPortTaskHandle_t taskHandle;
    uPortMutexHandle_t mutexHandle;
    int32_t eventQueueHandle;
} taskHandles_t;

typedef struct TaskConfig {
    const char *name;
    void *pParameter;
    taskHandles_t handles;
} taskConfig_t;

typedef int32_t (*taskStart_t)(taskConfig_t *taskConfig);
typedef void (*taskStop_t)(void);

typedef struct TaskRunner {
    taskStart_t startFunc;
    taskStop_t stopFunc;
    taskConfig_t config;
} taskRunner_t;

/* ----------------------------------------------------------------
 * EXTERNAL VARIABLES used in the application tasks
 * -------------------------------------------------------------- */

// serial number of the cellular module
extern char gSerialNumber[U_SECURITY_SERIAL_NUMBER_MAX_LENGTH_BYTES];

// This is the ubxlib deviceHandle for communicating with the celullar module
extern uDeviceHandle_t gDeviceHandle;

// This flag is set to true when the application's tasks should exit
extern bool gExitApp;

// This flag represents the network's registration status (false = unknown state)
extern bool gIsNetworkUp;

// application status
extern applicationStates_t gAppStatus;

// our framework tasks
extern taskRunner_t tasks[];

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */
bool isTaskRunning(const uPortMutexHandle_t mutex);
char *uStrDup(const char *src);

int32_t sendAppTaskMessage(int32_t taskId, const void *pMessage, size_t msgSize);

#endif