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
 * Signal Quality Task header
 *
 */

#ifndef _SIGNAL_QUALITY_TASK_H_
#define _SIGNAL_QUALITY_TASK_H_

/* ----------------------------------------------------------------
 * COMMON TASK FUNCTIONS
 * -------------------------------------------------------------- */
int32_t startSignalQualityTask(taskConfig_t *config);
void stopSignalQualityTask(void);

/* ----------------------------------------------------------------
 * QUEUE MESSAGE TYPE DEFINITIONS
 * -------------------------------------------------------------- */
typedef enum {
    MEASURE_SIGNAL_QUALTY_NOW,      // performs a signal quality measurement now
    SHUTDOWN_SIGNAL_QAULITY_TASK,   // shuts down the 'task' by ending the mutex, queue and task.
} signalQualityMsgType_t;

// Some message types are just a command, so they wont need a param/struct
typedef struct {
    signalQualityMsgType_t msgType;

    union {
        const char *topicName;
    } msg;
} signalQualityMsg_t;

#endif