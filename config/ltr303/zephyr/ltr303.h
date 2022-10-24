/*
 * Copyright (c) 2019 Actinius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LTR303_H_
#define ZEPHYR_DRIVERS_SENSOR_LTR303_H_

#include <sys/util.h>

#define LTR303_ALS_DATA_CH1_RESULT 0x88
#define LTR303_ALS_DATA_CH0_RESULT 0x8A
#define LTR303_REG_CONTR 0x80
#define LTR303_REG_MEASURE 0x85
#define LTR303_REG_MANUFACTURER_ID 0x87
#define LTR303_REG_DEVICE_ID 0x86

#define LTR303_MANUFACTURER_ID_VALUE 0x0005
#define LTR303_DEVICE_ID_VALUE 0x00A0

#define LTR303_ALS_INTEGRATE_TIME_200MS 0x10
#define LTR303_ALS_MEASURE_RATE_1000MS 0x02

#define LTR303_GAIN_MASK 0x1F
#define LTR303_MEAS_RATE_MASK 0x3F

#define LTR303_ACTIVE_MODE 0x01
#define LTR303_GAIN_1X 0x00


struct ltr303_data {
	const struct device *i2c;
	uint16_t ch0_sample;
	uint16_t ch1_sample;
};

#endif /* _SENSOR_LTR303_ */
