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
 * A simple demo application showing how to perform 
 * a cellular scan.
 *
*/

#include <string.h>
#include <stdio.h>

#include "ubxlib.h"

#include "leds.h"
#include "buttons.h"

#define BROKER_NAME "xxx"
#define ACCOUNT_NAME "xxx"
#define ACCOUNT_PASSWORD "xxx"

// configuring the cellular module to be used 
static uDeviceType_t gDeviceType = U_DEVICE_TYPE_CELL;
static const uNetworkCfgCell_t gNetworkCfg = {
    .type = U_NETWORK_TYPE_CELL,
    .pApn = "tsiot",       // Thingstream SIM, use NULL for default
    .timeoutSeconds = 240  // Connection timeout in seconds
};
static const uNetworkType_t gNetworkType = U_NETWORK_TYPE_CELL;

bool done;
uDeviceCfg_t gDeviceCfg;
int curr_led = 0;

// TODO check if this memory allocation is done correctly
char topic[32]; 
char buffer[25];
uMqttClientContext_t *pContext;

// Function to quickly handling the change of led color
void ledChangeColor(int newColorId){
    ledSet(curr_led, false);
    curr_led = newColorId;
    ledSet(curr_led, true);
}

// Function to handle the press of buttons
void button_pressed(int buttonNo, uint32_t holdTime)
{
    if (!holdTime) {
        // while pressed DO nothing
    } else {
        if (buttonNo == 1){
            // Turn the device off
            done = true;
        }
        else{
            printf("Button %d up. Hold time: %u ms\n", buttonNo, holdTime);
            sendMqttMessage("Hello from the button");
        }

    }
}

// Function to send message
void sendMqttMessage(char* message){
        snprintf(buffer, sizeof(buffer), message);
        ledChangeColor(2);
        uMqttClientPublish(pContext, topic, buffer,
                    strlen(buffer),
                    U_MQTT_QOS_EXACTLY_ONCE,
                    false);
        ledChangeColor(1);
}

void main()
{
    // Remove the line below if you want the log printouts from ubxlib
    //uPortLogOff();
    // Initiate ubxlib
    uPortInit();
    uDeviceInit();

    // Init buttons 
    if (!buttonsInit(button_pressed)) {
        printf("* Failed to initiate buttons\n");
    }
    // Init leds
    if (!ledsInit()) {
        printf("* Failed to initiate leds\n");
    }
    
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
            ledSet(curr_led, true);
            pContext = pUMqttClientOpen(deviceHandle, NULL);
            if (pContext != NULL) {
                uMqttClientConnection_t connection = U_MQTT_CLIENT_CONNECTION_DEFAULT;
                
                connection.pBrokerNameStr = BROKER_NAME;
                connection.pUserNameStr = ACCOUNT_NAME;
                connection.pPasswordStr = ACCOUNT_PASSWORD;
                if (uMqttClientConnect(pContext, &connection) == 0) {
                    ledChangeColor(1);

                    // Get a unique topic name for this test
                    uSecurityGetSerialNumber(deviceHandle, topic);
                    if (topic[0] == '"') {
                        // Remove quotes
                        size_t len = strlen(topic);
                        memmove(topic, topic + 1, len);
                        topic[len - 2] = 0;
                    }
                    if (uMqttClientSubscribe(pContext, topic,
                                             U_MQTT_QOS_EXACTLY_ONCE)) {
                        printf("----------------------------------------------\n");
                        printf("To view the mqtt messages from this device use:\n");
                        printf("mosquitto_sub -h %s -t %s -v\n", BROKER_NAME, topic);
                        printf("To send mqtt messages to this device use:\n");
                        printf("mosquitto_pub -h %s -t %s -m message\n", BROKER_NAME, topic);
                        printf("Send message \"exit\" to disconnect\n");
                        done = false;
                        while (!done) {       

                            uPortTaskBlock(1000);
                        }
                    } else {
                        printf("* Failed to subscribe to topic: %s\n", topic);
                    }
                    uMqttClientDisconnect(pContext);
                } else {
                    printf("* Failed to connect to the mqtt broker\n");
                }

            } else {
                printf("* Failed to create mqtt instance !\n ");
            }
            ledSet(curr_led, false);
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
