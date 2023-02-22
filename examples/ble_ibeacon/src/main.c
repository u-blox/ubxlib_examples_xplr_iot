/*
 * Copyright 2023 u-blox
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
 * A demo on setting up an iBeacon using ubxlib GAP/GATT apis.
 *
 * IMPORTANT! ubxlib version 1.2 or later is required.
 *
 */

#include <stdio.h>

#include "ubxlib.h"

#define ADV_INTERVAL_MS 1000
uint8_t ibeacon_data[] = {
    0x4c, 0x00,              // Apple ID
    0x02, 0x15,              // iBeacon Type and size
    0x01, 0x02, 0x03, 0x04,  // Dummy UUID
    0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C,
    0x0D, 0x0E, 0x0F, 0x10,
    0x00, 0x01,              // Major
    0x00, 0x02,              // Minor
    0xC3                     // RSSI
};

uDeviceHandle_t gDeviceHandle;
uDeviceCfg_t gDeviceCfg;

void main()
{
    static uNetworkCfgBle_t networkCfg = {
        .type = U_NETWORK_TYPE_BLE,
        .role = U_BLE_CFG_ROLE_PERIPHERAL,
        .spsServer = false,
    };

    // Remove the line below if you want the log printouts from ubxlib
    uPortLogOff();
    // Initiate ubxlib
    uPortInit();
    uDeviceInit();
    // And the U-blox module
    int32_t errorCode;
    uDeviceGetDefaults(U_DEVICE_TYPE_SHORT_RANGE, &gDeviceCfg);
    printf("\nInitiating the module...\n");
    errorCode = uDeviceOpen(&gDeviceCfg, &gDeviceHandle);
    if (errorCode == 0) {
        printf("Starting BLE...\n");
        errorCode = uNetworkInterfaceUp(gDeviceHandle, networkCfg.type, &networkCfg);
        if (errorCode == 0) {
            // Setup the iBeacon advertisement
            uint8_t advData[32];
            uBleGapAdvConfig_t advCfg = {
                .minIntervalMs = 200,
                .maxIntervalMs = 200,
                .connectable = false,
                .pRespData = "",     // Want empty response advertisement
                .respDataLength = 1, // Just the 0
                .pAdvData = advData
            };
            advCfg.advDataLength = uBleGapSetAdvData(NULL,
                                                     ibeacon_data, sizeof(ibeacon_data),
                                                     advData, sizeof(advData));
            errorCode = uBleGapAdvertiseStart(gDeviceHandle, &advCfg);
            if (errorCode == 0) {
                printf("Started iBeacon advertising\n");
                char mac[U_SHORT_RANGE_BT_ADDRESS_SIZE];
                uBleGapGetMac(gDeviceHandle, mac);
                printf("Mac address is: %s\n", mac);
            } else {
                printf("* Failed to start advertising: %d\n", errorCode);
            }
        } else {
            printf("* Failed to bring up the network: %d\n", errorCode);
        }
    } else {
        printf("* Failed to initiate the module: %d\n", errorCode);
    }
}
