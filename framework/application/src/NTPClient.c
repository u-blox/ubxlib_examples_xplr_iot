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
 * Simple function to return the clock time from a NTP Server
 *
 */

#include <time.h>

#include "common.h"
#include "ntpclient.h"
#include "..\include\zephyr\sys\byteorder.h"

/* ----------------------------------------------------------------
 * DEFINES
 * -------------------------------------------------------------- */

#define ntohl(x)   sys_be32_to_cpu(x)

// Timeout for waiting for NTP is 20 seconds, using a 100ms tick
#define NTP_TIMEOUT 200
#define NTP_SERVER "time.google.com"

#define NTP_TIMESTAMP_DELTA 2208988800ull

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static bool haveDataToRead = false;

typedef struct
{
  uint8_t li_vn_mode;      // Eight bits. li, vn, and mode.
                           // li.   Two bits.   Leap indicator.
                           // vn.   Three bits. Version number of the protocol.
                           // mode. Three bits. Client will pick mode 3 for client.

  uint8_t stratum;         // Eight bits. Stratum level of the local clock.
  uint8_t poll;            // Eight bits. Maximum interval between successive messages.
  uint8_t precision;       // Eight bits. Precision of the local clock.

  uint32_t rootDelay;      // 32 bits. Total round trip delay time.
  uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
  uint32_t refId;          // 32 bits. Reference clock identifier.

  uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
  uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

  uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
  uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

  uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
  uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

  uint32_t txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
  uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.

} ntp_packet;              // Total: 384 bits or 48 bytes.

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void dataCallback(void *pParam)
{
    haveDataToRead = true;
}

static bool waitForSocketData(void)
{
    for(int i = 0; !gExitApp && !haveDataToRead && i < NTP_TIMEOUT; i++)
        uPortTaskBlock(100);

    return haveDataToRead;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/// @brief Gets the time from the NTP Service
/// @return zero on failure, otherwise the unix epoch time
int64_t getNTPTime(void)
{
    ntp_packet packet = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    memset( &packet, 0, sizeof( ntp_packet ) );

    // Set the first byte's bits to 00,011,011 for li = 0, vn = 3, and mode = 3.
    *( ( char * ) &packet + 0 ) = 0x1b;

    uSockDescriptor_t sock = uSockCreate(gDeviceHandle, U_SOCK_TYPE_DGRAM, U_SOCK_PROTOCOL_UDP);
    if(sock < 0) {
        writeLog("Failed to create UDP socket: %d", sock);
        return sock;
    }

    haveDataToRead = false;
    uSockRegisterCallbackData(sock, &dataCallback, NULL);

    time_t time = 0;
    uSockAddress_t remoteAddress;
    int32_t errorCode = uSockGetHostByName(gDeviceHandle, NTP_SERVER, &(remoteAddress.ipAddress));
    if (errorCode == 0) {
        remoteAddress.port = 123;

        int64_t errorCode = uSockSendTo(sock, &remoteAddress, &packet, sizeof(ntp_packet));
        if (errorCode > 0) {
            if (waitForSocketData()) {
                errorCode = uSockReceiveFrom(sock, NULL, &packet, sizeof( ntp_packet ));
                if (errorCode > 0) {
                    writeLog("Received NTP time from server");
                    packet.txTm_s = ntohl(packet.txTm_s);
                    time = ( time_t ) ( packet.txTm_s - NTP_TIMESTAMP_DELTA );
                } else {
                    writeLog("Failed to read NTP Packet from server: %d", errorCode);
                }
            } else {
                writeLog("Waiting for NTP response timed out.");
            }
        } else {
            writeLog("Failed to send NTP Packet to server: %d", errorCode);
        }
    } else {
        writeLog("Failed to get Time Service IP Address: %d", sock);
    }

    errorCode = uSockClose(sock);
    if (errorCode != 0) {
        writeLog("Failed to close socket: %d", errorCode);
    }

    return time;
}