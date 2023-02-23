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
 * A demo on how to set up a NUS (Nordic Uart Service) server or client
 * using ubxlib.
 *
 * IMPORTANT! ubxlib version 1.2 or later is required.
 *
 */

#include <stdio.h>
#include <string.h>

#include "ubxlib.h"

#include "leds.h"

#define SERVER_NAME "NUS-Demo-Server"
#define SOLID_LED  RED_LED
#define BLINK_LED  BLUE_LED

// This application can run as either server (peripheral) or client (central).
// Choose here which to run.
bool gActAsServer = true;

static char gPeerMac[U_SHORT_RANGE_BT_ADDRESS_SIZE] = {0};
static char gPeerResponse[100] = {0};

char gLocalMac[U_SHORT_RANGE_BT_ADDRESS_SIZE] = {0};

uDeviceHandle_t gDeviceHandle;
uDeviceCfg_t gDeviceCfg;

// Convert mac address without colon to one with
const char *colonMac(const char *ubxMac)
{
    static char mac[U_SHORT_RANGE_BT_ADDRESS_SIZE + 5];
    int pos = 0;
    for (int i = 0; i < 12; i++) {
        mac[pos++] = ubxMac[i];
        if (i % 2) {
            mac[pos++] = ':';
        }
    }
    mac[pos - 1] = 0;
    return mac;
}

// Callback for scan response.
static bool scanResponseCb(uBleScanResult_t *pScanResult)
{
    if (strstr(pScanResult->name, SERVER_NAME)) {
        strncpy(gPeerMac, pScanResult->address, sizeof(gPeerMac));
        return false;
    }
    return true;
}

// Callback for incoming NUS messages from client or server.
static void peerIncomingCb(uint8_t *pValue, uint8_t valueSize)
{
    uint8_t maxSize = sizeof(gPeerResponse) - 1;
    if (valueSize > maxSize) {
        valueSize = maxSize;
    }
    memcpy(gPeerResponse, pValue, valueSize);
    gPeerResponse[valueSize] = 0;
}

// Run as a NUS server.
static void server()
{
    // Set up proper advertisement data.
    uint8_t respData[32];
    uint8_t advData[32];
    uBleGapAdvConfig_t advCfg = {
        .minIntervalMs = 200,
        .maxIntervalMs = 200,
        .connectable = true,
        .maxClients = 1,
        .pRespData = respData,
        .pAdvData = advData
    };
    advCfg.respDataLength = uBleNusSetAdvData(respData, sizeof(respData));
    advCfg.advDataLength = uBleGapSetAdvData(SERVER_NAME,
                                             NULL, 0,
                                             advData, sizeof(advData));
    if (advCfg.respDataLength > 0) {
        printf("Server initiated.\n");
        printf("Connect using e.g. the nRf toolbox app in a phone,\n");
        printf("  using the \"Utils services UART\".\n");
        printf("Server address is: %s (%s)\n", gLocalMac, colonMac(gLocalMac));
        printf("Waiting for connections....\n");
        uBleGapAdvertiseStart(gDeviceHandle, &advCfg);
        uBleNusInit(gDeviceHandle, NULL, peerIncomingCb);
    } else {
        printf("* Failed to start NUS server");
        return;
    }
    ledsInit();
    bool ledIsOn = true;
    bool ledBlinking = false;
    ledSet(SOLID_LED, ledIsOn);
    while (true) {
        char response[20] = {0};
        if (gPeerResponse[0] != 0) {
            // Client sent a message, see if it is recognizable.
            printf("Incoming command: %s\n", gPeerResponse);
            if (strstr(gPeerResponse, "hello")) {
                snprintf(response, sizeof(response), "Hello from server!");
            } else if (strstr(gPeerResponse, "led")) {
                if (ledBlinking) {
                    ledBlink(BLINK_LED, 0, 0);
                    ledBlinking = false;
                }
                ledIsOn = !ledIsOn;
                ledSet(SOLID_LED, ledIsOn);
                snprintf(response, sizeof(response), "Led is %s", ledIsOn ? "on" : "off");
            } else if (strstr(gPeerResponse, "blink")) {
                if (ledIsOn) {
                    ledIsOn = false;
                    ledSet(SOLID_LED, ledIsOn);
                }
                ledBlinking = !ledBlinking;
                ledBlink(BLINK_LED, ledBlinking ? 250 : 0, 250);
                snprintf(response, sizeof(response), "Led is %s", ledBlinking ? "blinking" : "off");
            } else {
                snprintf(response, sizeof(response), "Unknown command");
            }
            uBleNusWrite(response, strlen(response) + 1);
            gPeerResponse[0] = 0;
        } else {
            uPortTaskBlock(1000);
        }
    }
}

// Run as NUS client.
static void client()
{
    int32_t errorCode;
    printf("NUS client started, scanning for servers...\n");
    while (true) {
        gPeerMac[0] = 0;
        // Search for module running the as a server with this code.
        errorCode = uBleGapScan(gDeviceHandle,
                                U_BLE_GAP_SCAN_DISCOVER_ALL_ONCE,
                                true, 2000,
                                scanResponseCb);
        if (strlen(gPeerMac)) {
            printf("Connecting to %s...\n", gPeerMac);
            errorCode = uBleNusInit(gDeviceHandle, gPeerMac, peerIncomingCb);
            if (errorCode == 0) {
                uPortTaskBlock(2000);
                const char *com = "blink";
                uBleNusWrite(com, strlen(com) + 1);
                uPortTaskBlock(1000);
                if (gPeerResponse[0] != 0) {
                    printf("Server response: %s\n", gPeerResponse);
                }
                printf("Disconnecting\n");
                uBleNusDeInit();
                uPortTaskBlock(10000);
            } else {
                printf("* Connect failed: %d\n", errorCode);
            }
        }
    }
}

void main()
{
    static uNetworkCfgBle_t networkCfg = {
        .type = U_NETWORK_TYPE_BLE,
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
        networkCfg.role = gActAsServer ? U_BLE_CFG_ROLE_PERIPHERAL : U_BLE_CFG_ROLE_CENTRAL;
        errorCode = uNetworkInterfaceUp(gDeviceHandle, networkCfg.type, &networkCfg);
        if (errorCode == 0) {
            if (uBleGapGetMac(gDeviceHandle, gLocalMac) > 0) {
                printf("Local mac address: %s\n", gLocalMac);
            }
            if (gActAsServer) {
                server();
            } else {
                client();
            }
        } else {
            printf("* Failed to bring up the network: %d\n", errorCode);
        }
    } else {
        printf("* Failed to initiate the module: %d\n", errorCode);
    }
}
