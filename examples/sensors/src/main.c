
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
 * Simple demo program showing how to read some of
 * the sensors on the XPLR-IOT-1 board
 *
 */

#include <zephyr.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "sensors.h"


void main()
{
    initSensors();
    while (1) {
        printf("%s\n%s\n", pollTempSensor(), pollAccelerometer());
        k_msleep(2000);
    }
}
