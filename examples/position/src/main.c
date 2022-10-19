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
 * A simple demo application showing how to set up
 * and use a GNSS module using ubxlib.
 *
 */

#include <string.h>
#include <stdio.h>
#include <time.h>

#include "ubxlib.h"

#include "leds.h"

uDeviceCfg_t gDeviceCfg;

uNetworkCfgGnss_t gNetworkCfg = {
    .type = U_NETWORK_TYPE_GNSS
};

void main()
{
    ledsInit();
    // Remove the line below if you want the log printouts from ubxlib
    uPortLogOff();
    // Initiate ubxlib
    uPortInit();
    uDeviceInit();
    // And the U-blox GNSS module
    int32_t errorCode;
    uDeviceHandle_t deviceHandle;
    uDeviceGetDefaults(U_DEVICE_TYPE_GNSS, &gDeviceCfg);
    printf("\nInitiating the module...\n");
    errorCode = uDeviceOpen(&gDeviceCfg, &deviceHandle);
    if (errorCode == 0) {
        // Bring up the GNSS
        errorCode = uNetworkInterfaceUp(deviceHandle, U_NETWORK_TYPE_GNSS, &gNetworkCfg);
        if (errorCode == 0) {
            printf("Waiting for position.");
            uLocation_t location;
            int tries = 0;
            int64_t startTime = uPortGetTickTimeMs();
            ledBlink(RED_LED, 250, 250);
            do {
                printf(".");
                errorCode = uLocationGet(deviceHandle, U_LOCATION_TYPE_GNSS,
                                         NULL, NULL, &location, NULL);
            } while (errorCode == U_ERROR_COMMON_TIMEOUT && tries++ < 4);
            ledBlink(RED_LED, 0, 0);
            printf("\nWaited: %lld s\n", (uPortGetTickTimeMs() - startTime) / 1000);
            if (errorCode == 0) {
                ledSet(GREEN_LED, true);
                printf("Position: https://maps.google.com/?q=%d.%07d,%d.%07d\n",
                       location.latitudeX1e7 / 10000000, location.latitudeX1e7 % 10000000,
                       location.longitudeX1e7 / 10000000, location.longitudeX1e7 % 10000000);
                printf("Radius: %d m\n", location.radiusMillimetres / 1000);
                struct tm *t = gmtime(&location.timeUtc);
                printf("UTC Time: %4d-%02d-%02d %02d:%02d:%02d\n",
                       t->tm_year + 1900, t->tm_mon, t->tm_mday,
                       t->tm_hour, t->tm_min, t->tm_sec);
            } else if (errorCode == U_ERROR_COMMON_TIMEOUT) {
                printf("* Timeout\n");
                ledSet(RED_LED, true);
            } else {
              printf("* Failed to get position: %d\n", errorCode);
            }
            uNetworkInterfaceDown(deviceHandle, U_NETWORK_TYPE_GNSS);
        } else {
            printf("* Failed to bring up the GNSS: %d", errorCode);
        }
        uDeviceClose(deviceHandle, true);
    } else {
        printf("* Failed to initiate the module: %d", errorCode);
    }

    printf("\n== All done ==\n");

}
