
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
 * This example application shows how to use the ubxlib wifi captive
 * portal function for initial provisioning of SSID and password.
 *
 * IMPORTANT! ubxlib version 1.3 or later is required.
 *
 */

#include <stdio.h>
#include <string.h>

#include "ubxlib.h"

#include "leds.h"
#include "buttons.h"


#define PORTAL_NAME "UBXLIB_PORTAL"

uDeviceHandle_t gDeviceHandle;
bool gDoReset = false;

/* Button press callback */
static void button_pressed(int buttonNo, uint32_t holdTime)
{
    // Clear the current credentials and start the captive
    // portal after a long press of first button.
    if (buttonNo == 0 && holdTime > 2000) {
        gDoReset = true;
    }
}

static bool portalExit(uDeviceHandle_t deviceHandle)
{
    // Exit the portal when in connected mode if reset has been requested
    (void)deviceHandle;
    return gDoReset;
}

/* Connect to an access point or start the captive portal */
void doConnect()
{
    int32_t errorCode = -1;
    static uNetworkCfgWifi_t networkCfg = {
        .type = U_NETWORK_TYPE_WIFI,
        .mode = U_WIFI_MODE_STA,
    };
    ledSet(RED_LED, false);
    ledSet(GREEN_LED, false);
    ledSet(RED_LED, false);
    if (uWifiStationHasStoredConfig(gDeviceHandle)) {
        // Credentials have been saved earlier, try to connect to the access point.
        ledBlink(GREEN_LED, 250, 250);
        errorCode = uNetworkInterfaceUp(gDeviceHandle, U_NETWORK_TYPE_WIFI, &networkCfg);
    }
    if (errorCode == 0) {
        printf("Connected to WiFi\n");
    } else {
        printf("- Failed to bring up WiFi, starting captive portal.\n");
        printf("Use a phone or PC to connect to the WiFi access point\n");
        printf("with the name \"%s\".\n", PORTAL_NAME);
        ledBlink(BLUE_LED, 250, 250);
        errorCode = uWifiCaptivePortal(gDeviceHandle, PORTAL_NAME, NULL, NULL);
    }
    ledBlink(GREEN_LED, 0, 0);
    if (errorCode == 0) {
        // Connected
        ledSet(BLUE_LED, false);
        ledBlink(GREEN_LED, 0, 0);
        ledSet(GREEN_LED, true);
        int32_t sock = uSockCreate(gDeviceHandle,
                                   U_SOCK_TYPE_STREAM,
                                   U_SOCK_PROTOCOL_TCP);
        if (sock >= 0) {
            // Show the received ip address
            uSockAddress_t localAddr;
            char addrStr[15];
            uSockGetLocalAddress(sock, &localAddr);
            uSockIpAddressToString(&(localAddr.ipAddress), addrStr, sizeof(addrStr));
            uPortLog("Connected. Local address: %s\n", addrStr);
            uSockClose(sock);
            // Start the web server also in this mode in order to enable possibility
            // to change the selected network also when connected, mainly for testing.
            while (uWifiCaptivePortal(gDeviceHandle, NULL, NULL, portalExit) == 0) {
                gDoReset = false;
            }
            gDoReset = true;
        }
    }
    if (errorCode != 0) {
        printf("* Captive portal failed: %d\n", errorCode);
        ledSet(BLUE_LED, false);
        ledSet(GREEN_LED, false);
        ledBlink(RED_LED, 100, 100);
        uPortTaskBlock(5000);
        ledBlink(RED_LED, 0, 0);
        gDoReset = true;
    }
}

void main()
{
    if (!buttonsInit(button_pressed)) {
        printf("* Failed to initiate buttons\n");
    }
    if (!ledsInit()) {
        printf("* Failed to initiate leds\n");
    }

    // Remove the line below if you want the log printouts from ubxlib
    uPortLogOff();
    // Initiate ubxlib
    uPortInit();
    uDeviceInit();
    // And the U-blox module
    int32_t errorCode;
    static uDeviceCfg_t deviceCfg;
    uDeviceGetDefaults(U_DEVICE_TYPE_SHORT_RANGE, &deviceCfg);
    printf("\nInitiating the module...\n");
    errorCode = uDeviceOpen(&deviceCfg, &gDeviceHandle);
    if (errorCode == 0) {
        // First connect
        doConnect();
        while (true) {
            if (gDoReset) {
                gDoReset = false;
                printf("Reseting credentials...\n");
                uNetworkInterfaceDown(gDeviceHandle, U_NETWORK_TYPE_WIFI);
                uWifiStationStoreConfig(gDeviceHandle, true);
                doConnect();
            } else {
                uPortTaskBlock(1000);
            }
        }
    } else {
        printf("* Failed to start the module: %d\n", errorCode);
    }
}
