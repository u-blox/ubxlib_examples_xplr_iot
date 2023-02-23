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


uDeviceHandle_t gDeviceHandle;
uDeviceCfg_t gDeviceCfg;

static bool scanResponseCb(uBleScanResult_t *pScanResult)
{
    printf("%s;%s;", pScanResult->address, pScanResult->name);
    for (int i = 0; i < pScanResult->dataLength; i++) {
        printf("%02X", pScanResult->data[i]);
    }
    printf(";%d\n", pScanResult->rssi);
    return true;
}

void main()
{
    static uNetworkCfgBle_t networkCfg = {
        .type = U_NETWORK_TYPE_BLE,
        .role = U_BLE_CFG_ROLE_CENTRAL,
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
            errorCode = uBleGapScan(gDeviceHandle,
                                    U_BLE_GAP_SCAN_DISCOVER_ALL_ONCE,
                                    true, 10000,
                                    scanResponseCb);
            if (errorCode != 0) {
                printf("* Failed to start scanning: %d", errorCode);
            }
        } else {
            printf("* Failed to bring up the network: %d\n", errorCode);
        }
    } else {
        printf("* Failed to initiate the module: %d\n", errorCode);
    }
    printf("Done\n");
}
