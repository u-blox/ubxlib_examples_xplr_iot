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
 * LED Task header
 *
 */

#ifndef _LEDTASK_H_
#define _LEDTASK_H_

/* ----------------------------------------------------------------
 * PUBLIC TYPE DEFINITIONS
 * -------------------------------------------------------------- */

// LED configuration
typedef struct {
    int32_t n;          // LED number
    int32_t period;     // time period
    int32_t duty;       // duty cyle of it being on. 0 = off, 50 = half flash, 100 = on
    int32_t timer;      // timer position of the led. 0 = start, timer <= duty: on
    bool invert;        // invert the duty
    bool state;         // state of LED
    bool priority;      // Priority LED
} ledCfg_t;

/* ----------------------------------------------------------------
 * DEFINITIONS
 * -------------------------------------------------------------- */

#define ledTick_ms 10

// Some LED Examples   {LED#, period, duty, pos, invert, state, priority}
#define ledRedOn       {0, 1000, 1000, 0, false, false, false}
#define ledRedOff      {0, 1000, 0,    0, false, false, false}
#define ledGreenOn     {1, 1000, 1000, 0, false, false, false}
#define ledGreenOff    {1, 1000, 0,    0, false, false, false}
#define ledBlueOn      {2, 1000, 1000, 0, false, false, false}
#define ledBlueOff     {2, 1000, 00,   0, false, false, false}

#define ledRedFlash    {0, 100,  50,   0, false, false, false}
#define ledGreenFlash  {1, 100,  50,   0, false, false, false}
#define ledBlueFlash   {2, 100,  50,   0, false, false, false}

#define ledRedPulse    {0, 1000, 750,  0, false, false, false}
#define ledGreenPulse  {1, 1000, 750,  0, false, false, false}
#define ledBluePulse   {2, 1000, 750,  0, false, false, false}

#define ledRedBlink    {0, 1000, 200,  0, false, false, false}
#define ledGreenBlink  {1, 1000, 200,  0, false, false, false}
#define ledBlueBlink   {2, 1000, 200,  0, false, false, false}

#define ledRedBlinkInv    {0, 1000, 200, 0, true, false, false}
#define ledGreenBlinkInv  {1, 1000, 200, 0, true, false, false}
#define ledBlueBlinkInv   {2, 1000, 200, 0, true, false, false}

#define ledRedPulseInv       {0, 1000, 750, 0, true,  false, false}
#define ledRedFastPulse      {0, 500,  100, 0, false, false, false}
#define ledGreenFastPulseInv {1, 500,  100, 0, true,  false, false}
#define ledRedFastPri        {0, 500,  200, 0, false, false, true}

/* ----------------------------------------------------------------
 * COMMON TASK FUNCTIONS
 * -------------------------------------------------------------- */
int32_t startLEDTask(taskConfig_t *config);
void stopLEDTask(void);

/* ----------------------------------------------------------------
 * TASK FUNCTIONS
 * -------------------------------------------------------------- */
void setManualLeds(ledCfg_t l1, ledCfg_t l2, ledCfg_t l3);

/* ----------------------------------------------------------------
 * QUEUE MESSAGE TYPE DEFINITIONS
 * -------------------------------------------------------------- */
typedef struct {
    int32_t LEDNUmber;
    bool state;
} LEDConfigMsg_t;

#endif