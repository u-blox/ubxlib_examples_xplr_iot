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

#include <kernel.h>
#include <device.h>
#include <drivers/gpio.h>

#include "buttons.h"

static struct k_sem button_semaphore;
static const struct gpio_dt_spec buttons[] = {
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0}),
};
static const int button_cnt = sizeof(buttons) / sizeof(buttons[0]);
static struct gpio_callback button_cb_data;
static button_cb_t button_cb = NULL;

static void buttonThread(void);
K_THREAD_DEFINE(buttonThread_id, 1024, buttonThread, NULL, NULL, NULL, 7, 0, K_TICKS_FOREVER);

void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    for (int i = 0; i < button_cnt; i++) {
        gpio_remove_callback(buttons[i].port, &button_cb_data);
    }
    k_sem_give(&button_semaphore);
}

bool buttonsInit(button_cb_t cb)
{
    bool ok = true;
    gpio_init_callback(&button_cb_data, button_isr,
                       BIT(buttons[0].pin) | BIT(buttons[1].pin));
    for (int i = 0; i < button_cnt && ok; i++) {
        ok = device_is_ready(buttons[i].port) &&
             gpio_pin_configure_dt(&buttons[i], GPIO_INPUT) == 0 &&
             gpio_pin_interrupt_configure_dt(&buttons[i], GPIO_INT_EDGE_TO_ACTIVE) == 0;
        gpio_add_callback(buttons[i].port, &button_cb_data);
    }
    if (ok) {
        button_cb = cb;
        k_sem_init(&button_semaphore, 0, 1);
        k_thread_start(buttonThread_id);
    }
    return ok;
}

static void buttonThread(void)
{
    while (true) {
        k_sem_take(&button_semaphore, K_FOREVER);
        int buttonNo = gpio_pin_get(buttons[0].port, buttons[0].pin) ? 0 : 1;
        uint32_t start = k_uptime_get_32();
        k_sleep(K_MSEC(100));  // Debounce

        button_cb(buttonNo, 0);
        while (gpio_pin_get(buttons[buttonNo].port, buttons[buttonNo].pin)) {
            k_sleep(K_MSEC(10));
        }

        button_cb(buttonNo, k_uptime_get_32() - start);

        for (int i = 0; i < button_cnt; i++) {
            gpio_add_callback(buttons[i].port, &button_cb_data);
        }
    }
}
