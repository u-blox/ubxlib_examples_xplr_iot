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
 * Registration Task header
 *
 */
#ifndef _REGISTRATION_TASK_H_
#define _REGISTRATION_TASK_H_

/* ----------------------------------------------------------------
 * COMMON TASK FUNCTIONS
 * -------------------------------------------------------------- */
int32_t startNetworkRegistrationTask(taskConfig_t *config);
void stopNetworkRegistrationTask(void);

/* ----------------------------------------------------------------
 * QUEUE MESSAGE TYPE DEFINITIONS
 * -------------------------------------------------------------- */
typedef struct {
    bool state;
} registrationMsg_t;

#endif