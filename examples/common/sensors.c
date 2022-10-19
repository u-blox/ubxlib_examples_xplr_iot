
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
#include <drivers/sensor.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr.h>

const struct device *gpBme280Dev;
const struct device *gpLis2dhDevice;

#define INIT_SENSOR(sensor_name, p)                                                                           \
  {                                                                                                           \
    p = DEVICE_DT_GET_ANY(sensor_name);                                                                       \
    if (p && device_is_ready(p)) {                                                                            \
      printf("Found device \"%s\", on I2C address 0x%02x \n", p->name, DT_REG_ADDR(DT_INST(0, sensor_name))); \
    }                                                                                                         \
  }

static float decodeVal(struct sensor_value *sensVal)
{
    return (float)(sensVal->val1 + sensVal->val2 / 1000000.0);
}

static char temp_buffer[100];
static char acc_buffer[100];

const char *pollTempSensor()
{
    temp_buffer[0] = 0;
    if (gpBme280Dev && sensor_sample_fetch(gpBme280Dev) == 0) {
        struct sensor_value temp, press, humidity;
        sensor_channel_get(gpBme280Dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        sensor_channel_get(gpBme280Dev, SENSOR_CHAN_PRESS, &press);
        sensor_channel_get(gpBme280Dev, SENSOR_CHAN_HUMIDITY, &humidity);
        snprintf(temp_buffer, sizeof(temp_buffer),
                 "Temp: %.2f C, Press: %.2f hPa, Humidity: %.2f %%",
                 decodeVal(&temp), decodeVal(&press) * 10, decodeVal(&humidity));
    }
    return temp_buffer;
}

const char *pollAccelerometer()
{
    acc_buffer[0] = 0;
    if (gpLis2dhDevice && sensor_sample_fetch(gpLis2dhDevice) >= 0) {
        struct sensor_value accel[3];
        if (sensor_channel_get(gpLis2dhDevice, SENSOR_CHAN_ACCEL_XYZ, accel) == 0) {
            snprintf(acc_buffer, sizeof(acc_buffer),
                     "Accel: X = %.3f g, Y = %.3f g, Z = %.3f g",
                     decodeVal(&accel[0]) / 10, decodeVal(&accel[1]) / 10, decodeVal(&accel[2]) / 10);
        }
    }
    return acc_buffer;
}

void initSensors()
{
    INIT_SENSOR(bosch_bme280, gpBme280Dev);
    INIT_SENSOR(st_lis2dh, gpLis2dhDevice);
}
