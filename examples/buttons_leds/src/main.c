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
 * Example showing how to use the buttons and leds in the XPLR-IOT-1
 *
 */

#include <stdio.h>

#include "leds.h"
#include "buttons.h"

int curr_led = 0;

void button_pressed(int buttonNo, uint32_t holdTime)
{
    if (!holdTime) {
        printf("Button %d down\n", buttonNo);
        if (buttonNo == 0) {
            // Led off on down
            ledSet(curr_led, false);
        } else {
            // Second button, switch color
            ledSet(curr_led, false);
            curr_led = (curr_led + 1) % 3;
            ledSet(curr_led, true);
        }
    } else {
        printf("Button %d up. Hold time: %u ms\n", buttonNo, holdTime);
        if (buttonNo == 0) {
            // Led on on up
            ledSet(curr_led, true);
        }
    }
}

void main(void)
{
    if (!buttonsInit(button_pressed)) {
        printf("* Failed to initiate buttons\n");
    }
    if (!ledsInit()) {
        printf("* Failed to initiate leds\n");
    }
    ledSet(curr_led, true);
    printf("Press the buttons!\n");

}
