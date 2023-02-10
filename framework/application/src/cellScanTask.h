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

#ifndef _CELLSCAN_TASK_H_
#define _CELLSCAN_TASK_H_

/* ----------------------------------------------------------------
 * COMMON TASK FUNCTIONS
 * -------------------------------------------------------------- */
int32_t startCellScanTask(taskConfig_t *config);
void stopCellScanTask(void);

/* ----------------------------------------------------------------
 * TASK FUNCTIONS
 * -------------------------------------------------------------- */
int32_t startNetworkScan(const char *pSerialNumber);

/* ----------------------------------------------------------------
 * QUEUE MESSAGE TYPE DEFINITIONS
 * -------------------------------------------------------------- */
typedef enum {
    START_CELL_SCAN,            // starts the network cell scanning
    STOP_CELL_SCAN,             // stops the network cell scanning
    SHUTDOWN_CELL_SCAN_TASK,    // shuts down the 'task' by ending the mutex, queue and task.
} cellScanMsgType_t;

// Some message types are just a command, so they wont need a param/struct
typedef struct {
    cellScanMsgType_t msgType;

    union {
        const char *topicName;
    } msg;
} cellScanMsg_t;

#endif