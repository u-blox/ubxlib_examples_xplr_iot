
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
 * network communication using ubxlib. Uses sockets
 * for sending and receiving data.
 *
 */

#include <string.h>
#include <stdio.h>

#include "ubxlib.h"

// Change the line below based on which type of module you want to use
#if 1
// Cellular network
static uDeviceType_t gDeviceType = U_DEVICE_TYPE_CELL;
static const uNetworkCfgCell_t gNetworkCfg = {
    .type = U_NETWORK_TYPE_CELL,
    .pApn = "tsiot",       // Thingstream SIM, use NULL for default
    .timeoutSeconds = 240  // Connection timeout in seconds
};
static const uNetworkType_t gNetworkType = U_NETWORK_TYPE_CELL;
#else
// WiFi network
static uDeviceType_t gDeviceType = U_DEVICE_TYPE_SHORT_RANGE;
static const uNetworkCfgWifi_t gNetworkCfg = {
    .type = U_NETWORK_TYPE_WIFI,
    .pSsid = "SSID",      // Wifi SSID - replace with your SSID
    .authentication = 2,  // WPA/WPA2/WPA3
    .pPassPhrase = "???"  // WPA passphrase - replace with yours
};
static const uNetworkType_t gNetworkType = U_NETWORK_TYPE_WIFI;
#endif

uDeviceCfg_t gDeviceCfg;

void main()
{
    // Remove the line below if you want the log printouts from ubxlib
    uPortLogOff();
    // Initiate ubxlib
    uPortInit();
    uDeviceInit();
    // And the U-blox module
    int32_t errorCode;
    uDeviceHandle_t deviceHandle;
    uDeviceGetDefaults(gDeviceType, &gDeviceCfg);
    printf("\nInitiating the module...\n");
    errorCode = uDeviceOpen(&gDeviceCfg, &deviceHandle);
    if (errorCode == 0) {
        printf("Bringing up the network...\n");
        errorCode = uNetworkInterfaceUp(deviceHandle, gNetworkType, &gNetworkCfg);
        if (errorCode == 0) {
            // Send to and read back data from an echo server using ubxlib sockets
            uSockAddress_t address;
            uSockGetHostByName(deviceHandle, "ubxlib.redirectme.net",
                               &(address.ipAddress));
            address.port = 5055;
            int32_t sock = uSockCreate(deviceHandle,
                                       U_SOCK_TYPE_STREAM,
                                       U_SOCK_PROTOCOL_TCP);
            errorCode = uSockConnect(sock, &address);
            if (errorCode == 0) {
                // Send data over the socket
                const char message[] = "The quick brown fox jumps over the lazy dog.";
                size_t size = strlen(message);
                size_t sentSize = 0;
                int32_t res = 0;
                while (res >= 0 && sentSize < size) {
                    int32_t res = uSockWrite(sock, (void *)(message + sentSize),
                                             size - sentSize);
                    if (res > 0) {
                        sentSize += res;
                    }
                }
                // And read it back
                char buffer[64];
                size_t rxSize = 0;
                res = 0;
                while ((res >= 0) && (rxSize < sizeof(message) - 1)) {
                    res = uSockRead(sock, buffer + rxSize, sizeof(buffer) - rxSize);
                    if (res > 0) {
                        rxSize += res;
                    }
                }
                buffer[rxSize] = 0;
                printf("Received: %s\n", buffer);
            } else {
                printf("* Failed to connect: %d", errorCode);
            }
            printf("Closing down the network...\n");
            uNetworkInterfaceDown(deviceHandle, gNetworkType);
        } else {
            printf("* Failed to bring up the network: %d\n", errorCode);
        }
        uDeviceClose(deviceHandle, true);
    } else {
        printf("* Failed to initiate the module: %d\n", errorCode);
    }

    printf("\n== All done ==\n");

    while (1) {
        uPortTaskBlock(1000);
    }
}
