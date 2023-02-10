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
 * Example task for duplicating for your own tasks to be added to 
 * this cellular application framework.
 *
 */

#include "common.h"
#include "exampleTask.h"
// #include "otherheaders...h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */
#define EXAMPLE_DWELL_SECONDS 60

#define EXAMPLE_QUEUE_STACK_SIZE (U_PORT_EVENT_QUEUE_MIN_TASK_STACK_SIZE_BYTES)
#define EXAMPLE_QUEUE_PRIORITY 5

// not all tasks will have a task loop if it only uses a queue
#define EXAMPLE_TASK_STACK_SIZE (1 * 1024)
#define EXAMPLE_TASK_PRIORITY 5

/* ----------------------------------------------------------------
 * TASK COMMON VARIABLES
 * -------------------------------------------------------------- */
static bool exitTask = false;
static taskConfig_t *taskConfig;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief keepGoing function for the UBXLIB "keepGoing" callbacks
/// @param pParam not used here
/// @return true if the UBXLIB function should continue
static bool keepGoing(void *pParam)
{
    return !gExitApp && !exitTask;
}

static void queueHandler(void *pParam, size_t paramLengthBytes)
{
    // cast the incoming pParam to the proper param structure
    exampleMsg_t *params = (exampleMsg_t *) pParam;

    U_PORT_MUTEX_LOCK(TASK_MUTEX);

    // do the message handling...

    U_PORT_MUTEX_UNLOCK(TASK_MUTEX);
}

static void dwellTask(void)
{
    // multiply by 10 as the TaskBlock is 100ms
    int32_t count = EXAMPLE_DWELL_SECONDS * 10;
    for(int i = 0; keepGoing(NULL) && i < count; i++)
        uPortTaskBlock(100);
}

// Signal Quality task loop for reading the RSRP and RSRQ values
// and sending these values to the MQTT topic
static void taskLoop(void *pParameters)
{
    uPortMutexLock(TASK_MUTEX);
    while(keepGoing(NULL)) {

        // Your task code here //

        dwellTask();
    }

    uPortMutexUnlock(TASK_MUTEX);
    writeLog("%s Task finished.", taskConfig->name);
}

static int32_t initQueue(taskConfig_t *config)
{
    int32_t eventQueueHandle = uPortEventQueueOpen(&queueHandler,
                    TASK_NAME,
                    sizeof(exampleMsg_t),
                    EXAMPLE_QUEUE_STACK_SIZE,
                    EXAMPLE_QUEUE_PRIORITY,
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
                    EXAMPLE_TASK_STACK_SIZE,
                    NULL,
                    EXAMPLE_TASK_PRIORITY,
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

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Starts the Signal Quality task
/// @param pParam Any parameter to be sent to the starting of the task
/// @return zero if successfull, a negative number otherwise
int32_t startExampleTask(taskConfig_t *config)
{
    taskConfig = config;

    int32_t result = U_ERROR_COMMON_SUCCESS;

    writeLog("Starting the %s task...", TASK_NAME);
    CHECK_SUCCESS(initTask, config);
    CHECK_SUCCESS(initQueue, config);
    CHECK_SUCCESS(initMutex, config);

    return result;
}

void stopExampleTask(void)
{
    exitTask = true;
    writeLog("Stop %s task requested...", taskConfig->name);
}