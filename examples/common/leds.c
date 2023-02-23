
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

#include <device.h>
#include <drivers/gpio.h>
#include <kernel.h>

#include "leds.h"

#define VALID_LED(led_no) (led_no >= RED_LED && led_no <= BLUE_LED)

static void do_blink(void);
K_THREAD_DEFINE(thread_id, 1024, do_blink, NULL, NULL, NULL, 7, 0, K_TICKS_FOREVER);

#define LED_ON gpio_pin_set(leds[curr_led].port, leds[curr_led].pin, 0)
#define LED_OFF gpio_pin_set(leds[curr_led].port, leds[curr_led].pin, 1)
static struct gpio_dt_spec leds[] = {
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(led2), gpios, {0}),
};
#define LED_CNT (sizeof(leds) / sizeof(leds[0]))
static bool led_is_on[LED_CNT];

static struct k_sem blink_sem;
static bool blink_started = false;
static volatile bool blink_enabled = false;
static int curr_ledBlink = 0;
static uint32_t blink_ms[2] = { 0, 0 };

bool ledsInit(void)
{
    bool ok = true;
    for (int i = 0; ok && i < LED_CNT; i++) {
        ok = device_is_ready(leds[i].port) && gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT) == 0;
    }
    return ok;
}

bool ledSet(int led_no, bool on)
{
    bool ok = VALID_LED(led_no) &&
              gpio_pin_set(leds[led_no].port, leds[led_no].pin, on ? 0 : 1) == 0;
    if (ok) {
        led_is_on[led_no] = on;
    }
    return ok;
}

bool ledToggle(int led_no)
{
    return VALID_LED(led_no) && ledSet(led_no, !led_is_on[led_no]);
}

bool ledBlink(int led_no, uint32_t on_ms, uint32_t off_ms)
{
    bool ok = VALID_LED(led_no);
    if (ok) {
        if (blink_enabled && (on_ms == 0 || led_no != curr_ledBlink)) {
            // Make sure that possible sleep is interrupted in the thread
            k_thread_suspend(thread_id);
            blink_enabled = false;
            k_sleep(K_MSEC(1));
            // Resume now when blink is disabled
            k_thread_resume(thread_id);
            ledSet(curr_ledBlink, false);
        }
        if (on_ms > 0) {
            blink_ms[0] = on_ms;
            blink_ms[1] = off_ms;
            curr_ledBlink = led_no;
            if (!blink_started) {
                k_sem_init(&blink_sem, 0, 1);
                k_thread_start(thread_id);
                blink_started = true;
            }
            blink_enabled = true;
            k_sem_give(&blink_sem);
        } else {
            // Already blinking
            ok = false;
        }
    }
    return ok;
}

void do_blink(void)
{
    while (true) {
        k_sem_take(&blink_sem, K_FOREVER);
        while (blink_enabled) {
            for (int i = 0; i < 2 && blink_enabled; i++) {
                ledSet(curr_ledBlink, i == 0);
                k_sleep(K_MSEC(blink_ms[i]));
            }
        }
    }
}