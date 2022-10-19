
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

#include <stdint.h>
#include <stdbool.h>

#define RED_LED   0
#define GREEN_LED 1
#define BLUE_LED  2

/** Initiate led handling. Performs led gpio setup.
 * @return     Success or failure.
 */
bool ledsInit(void);

/** Set led state on or off.
 * @param   led_no Led index in accordance to definitions above.
 * @param   on     On or off.
 * @return         Success or failure.
 */
bool ledSet(int led_no, bool on);

/** Toggle led state.
 * @param   led_no Led index in accordance to definitions above.
 * @return         Success or failure.
 */
bool ledToggle(int led_no);

/** Start blinkin of a led.
 * @param   led_no Led index in accordance to definitions above.
 * @param   on_ms  Time in milliseconds during which the led is on.
 *                 Set to 0 to stop the blink.
 * @param   off_ms Time in milliseconds during which the led is off.
 * @return         Success or failure.
 */
bool ledBlink(int led_no, uint32_t on_ms, uint32_t off_ms);