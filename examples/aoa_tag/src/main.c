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
 * This example shows how to make an XPLR-IOT-1 act as an
 * angle of arrival BLE tag. The location of this tag can then be detected
 * by u-connectLocate https://www.u-blox.com/en/product/u-connectlocate
 * Version 2.0 or later of u-connectLocate is required.
 *
 */

#include <stdio.h>

#include "buttons.h"
#include "leds.h"
#include "ble_aoa.h"

#define ADV_IND_LED BLUE_LED

static uint16_t advIntervals[] = { 50, 100, 1000 };
static int currInt = -1;
static bool doAdvertise = true;

void updateInterval()
{
    currInt = (currInt + 1) % (sizeof(advIntervals) / sizeof(advIntervals[0]));
    uint16_t interval = advIntervals[currInt];
    bleAoaAdvertise(interval, interval, true);
    ledBlink(ADV_IND_LED, interval, interval);
    printf("New advertisment interval %d ms\n", interval);
}

void button_pressed(int buttonNo, uint32_t holdTime)
{
    if (!holdTime) {
        if (buttonNo == 0) {
            if (doAdvertise) {
                updateInterval();
            }
        } else {
            doAdvertise = !doAdvertise;
            uint16_t interval = doAdvertise ? advIntervals[currInt] : 0;
            bleAoaAdvertise(interval, interval, doAdvertise);
            ledBlink(ADV_IND_LED, interval, interval);
            printf("Turning %s advertising\n", doAdvertise ? "on" : "off");
        }
    }
}

void main(void)
{
    buttonsInit(button_pressed);
    ledsInit();
    if (bleAoaInit()) {
        updateInterval();
        printf("AoA tag started\n");
    } else {
        printf("* Failed to start Bluetooth\n");
    }
}
