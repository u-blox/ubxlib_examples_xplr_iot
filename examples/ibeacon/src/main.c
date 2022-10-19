
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
 * A simple demo application showing how to create
 * an iBeacon using the Zephyr BLE functionality in the Nora
 * host cpu of the XPLR-IOT-1.
 *
 * Please note thats this examples needs that the network cpu
 * is flashed with the correct firmware. Accomplished by using the command:
 *
 * do flash_net
 *
 * after a successful build of this example. This is only needed to be done once.
 *
 */


#include <stdio.h>
#include <bluetooth/bluetooth.h>

// iBeacon advertisement package
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
    BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA,
                  0x4c, 0x00,             // Apple ID
                  0x02, 0x15,             // iBeacon Type and size
                  0x01, 0x02, 0x03, 0x04, // Dummy UUID
                  0x05, 0x06, 0x07, 0x08,
                  0x09, 0x0A, 0x0B, 0x0C,
                  0x0D, 0x0E, 0x0F, 0x10,
                  0x00, 0x01,             // Major
                  0x00, 0x02,             // Minor
                  0xC3                    // RSSI
                 )
};

void main(void)
{
    int err;
    err = bt_enable(NULL);
    if (err) {
        printf("* Bluetooth init failed (err %d)\n", err);
    } else {
        err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad),
                              NULL, 0);
        if (err) {
            printf("* Failed to start advertising %d\n", err);
        } else {
            printf("iBeacon advertising started\n");
        }
    }
}
