
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
const struct device *gpLis2dhDev;
const struct device *gLtr303Dev;

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
static char light_buffer[25];

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
    if (gpLis2dhDev && sensor_sample_fetch(gpLis2dhDev) >= 0) {
        struct sensor_value accel[3];
        if (sensor_channel_get(gpLis2dhDev, SENSOR_CHAN_ACCEL_XYZ, accel) == 0) {
            snprintf(acc_buffer, sizeof(acc_buffer),
                     "Accel: X = %.3f g, Y = %.3f g, Z = %.3f g",
                     decodeVal(&accel[0]) / 10, decodeVal(&accel[1]) / 10, decodeVal(&accel[2]) / 10);
        }
    }
    return acc_buffer;
}

#define ALS_GAIN 1
#define ALS_INT 2
#define pFfactor .16

static int32_t convToLux(struct sensor_value *adc_val)
{
    int32_t newval, ch0, ch1;
    int32_t r1, r2, r3;

    ch0 = adc_val->val1;
    ch1 = adc_val->val2;
    r1 = (ch1 * 100);
    r2 = (ch0 + ch1);
    if ((r1 != 0) || (r2 != 0)) {
        r3 = r1 / r2;
    } else {
        r3 = 0;
    }
    if (r3 < 45) {
        newval = (1.7743 * ch0 + 1.1059 * ch1) / ALS_GAIN / ALS_INT / pFfactor;
    } else if ((r3 < 64) && (r3 >= 45)) {
        newval = (4.2785 * ch0 - 1.9548 * ch1) / ALS_GAIN / ALS_INT / pFfactor;
    } else if ((r3 < 85) && (r3 >= 64)) {
        newval = (.5926 * ch0 + .1185 * ch1) / ALS_GAIN / ALS_INT / pFfactor;
    } else {
        newval = 0;
    }
    return newval;
}

int32_t getLightSensor()
{
    struct sensor_value adc;
    int32_t luxVal = 0;
    if (gLtr303Dev && sensor_sample_fetch(gLtr303Dev) >= 0) {
        sensor_channel_get(gLtr303Dev, SENSOR_CHAN_LIGHT, &adc);
        luxVal = convToLux(&adc);
    }
    return luxVal;
}

const char *pollLightSensor()
{
    snprintf(light_buffer, sizeof(light_buffer), "Light = %d lux", getLightSensor());
    return light_buffer;
}

void sensorsInit()
{
    INIT_SENSOR(bosch_bme280, gpBme280Dev);
    INIT_SENSOR(st_lis2dh, gpLis2dhDev);
    INIT_SENSOR(ltr_303als, gLtr303Dev);
}
