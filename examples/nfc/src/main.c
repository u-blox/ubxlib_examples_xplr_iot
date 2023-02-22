
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
 * A simple demo application showing how to use the nfc capability of
 * the Nora B1 (nrf5340) in the XPLR-IOT-1. In this example a url ndef tag
 * is defined and can be detected by e.g. a mobile phone. This will then
 * start the browser in the phone, opening the specified url.
 *
 */

#include <stdio.h>
#include <kernel.h>

#include <nfc_t2t_lib.h>
#include <nfc/ndef/uri_msg.h>
#include <nfc/ndef/launchapp_msg.h>

#define FAIL(mess) { printf("* Failed %s\n", mess); return 1;}

const char *gUrl = "github.com/u-blox/ubxlib_examples_xplr_iot";

static void nfc_callback(void *context,
                         nfc_t2t_event_t event,
                         const uint8_t *data,
                         size_t data_length)
{
    ARG_UNUSED(context);
    ARG_UNUSED(data);
    ARG_UNUSED(data_length);

    switch (event) {
        case NFC_T2T_EVENT_FIELD_ON:
            printf("Nfc field on\n");
            break;
        case NFC_T2T_EVENT_FIELD_OFF:
            printf("Nfc field off\n");
            break;
        default:
            break;
    }
}

int main(void)
{
    static uint8_t ndefMsgBuf[256];
    if (nfc_t2t_setup(nfc_callback, NULL) != 0) {
        FAIL("to setup NFC");
    }

    uint32_t nDefLen = sizeof(ndefMsgBuf);
    if (nfc_ndef_uri_msg_encode(NFC_URI_HTTPS_WWW,
                                (const uint8_t *)gUrl, strlen(gUrl),
                                ndefMsgBuf, &nDefLen) != 0) {
        FAIL("to encode url");
    }

    if (nfc_t2t_payload_set(ndefMsgBuf, nDefLen) != 0) {
        FAIL("to set payload");
    }

    if (nfc_t2t_emulation_start() != 0 ) {
        FAIL("to start emulation");
    }

    printf("NFC emulation started. Url = %s\n", gUrl);
    printf("Put a phone with nfc enabled on top of the XPLR-IOT-1\n");

    return 0;
}
