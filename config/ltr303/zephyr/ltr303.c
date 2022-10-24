/*
 * Copyright (c) 2019 Actinius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ltr_303als

#include <device.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include <sys/__assert.h>
#include <version.h>

#include "ltr303.h"

LOG_MODULE_REGISTER(ltr303, CONFIG_SENSOR_LOG_LEVEL);

static struct ltr303_data ltr303_drv_data;

static int ltr303_reg_read(struct ltr303_data *drv_data, uint8_t reg,
                           uint16_t *val)
{
    uint8_t value[2];

    if (( reg == LTR303_ALS_DATA_CH1_RESULT) || ( reg == LTR303_ALS_DATA_CH0_RESULT)) {

        if (i2c_burst_read(drv_data->i2c, DT_INST_REG_ADDR(0),
                           reg, value, 2) != 0) {
            return -EIO;
        }

        *val = ((uint16_t)value[1] << 8) | value[0];
    } else { // other registers, 1 byte
        if (i2c_burst_read(drv_data->i2c, DT_INST_REG_ADDR(0),
                           reg, value, 1) != 0) {
            return -EIO;
        }

        *val = ((uint16_t) 0x00FF & value[0]);
    }

    return 0;
}

static int ltr303_reg_write(struct ltr303_data *drv_data, uint8_t reg,
                            uint8_t val)
{

    uint8_t tx_buf[2] = { reg, val };

    return i2c_write(drv_data->i2c, tx_buf, sizeof(tx_buf),
                     DT_INST_REG_ADDR(0));
}

static int ltr303_reg_update(struct ltr303_data *drv_data, uint8_t reg,
                             uint8_t mask, uint8_t val)
{

    return ltr303_reg_write(drv_data, reg, val);
}

static int ltr303_sample_fetch(const struct device *dev,
                               enum sensor_channel chan)
{
    struct ltr303_data *drv_data = dev->data;
    uint16_t value;

    __ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT);
    drv_data->ch1_sample = 0U;
    drv_data->ch0_sample = 0U;

    // from LTR-303ALS-01 - Read CH1 prior to CH0

    if (ltr303_reg_read(drv_data, LTR303_ALS_DATA_CH1_RESULT, &value) != 0) {
        return -EIO;
    }
    drv_data->ch1_sample = value;


    if (ltr303_reg_read(drv_data, LTR303_ALS_DATA_CH0_RESULT, &value) != 0) {
        return -EIO;
    }
    drv_data->ch0_sample = value;

    return 0;
}

static int ltr303_channel_get(const struct device *dev,
                              enum sensor_channel chan,
                              struct sensor_value *val)
{
    struct ltr303_data *drv_data = dev->data;

    if (chan != SENSOR_CHAN_LIGHT) {
        return -ENOTSUP;
    }

    val->val1 = drv_data->ch0_sample;
    val->val2 = drv_data->ch1_sample;

    return 0;
}

static const struct sensor_driver_api ltr303_driver_api = {
    .sample_fetch = ltr303_sample_fetch,
    .channel_get = ltr303_channel_get,
};

static int ltr303_chip_init(const struct device *dev)
{
    struct ltr303_data *drv_data = dev->data;
    uint16_t value;
#if KERNEL_VERSION_MAJOR < 3
    drv_data->i2c = device_get_binding("I2C_1");
#else
    drv_data->i2c = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(i2c1));
#endif

    if (drv_data->i2c == NULL) {
        LOG_ERR("Failed to get pointer to ltr303 device!");
        return -EINVAL;
    }

    if (ltr303_reg_read(drv_data, LTR303_REG_MANUFACTURER_ID, &value) != 0) {
        return -EIO;
    }

    if (value != LTR303_MANUFACTURER_ID_VALUE) {
        LOG_ERR("Bad manufacturer id 0x%x", value);
        return -ENOTSUP;
    }

    if (ltr303_reg_read(drv_data, LTR303_REG_DEVICE_ID, &value) != 0) {
        return -EIO;
    }

    if (value != LTR303_DEVICE_ID_VALUE) {
        LOG_ERR("Bad device id 0x%x", value);
        return -ENOTSUP;
    }

    if (ltr303_reg_update(drv_data, LTR303_REG_CONTR, LTR303_GAIN_MASK, (LTR303_ACTIVE_MODE ||
                                                                         LTR303_GAIN_1X)) != 0) {
        LOG_ERR("Failed to set ALS Gain setting, Activate ALS Mode");
        return -EIO;
    }

    if (ltr303_reg_update(drv_data, LTR303_REG_MEASURE, LTR303_MEAS_RATE_MASK,
                          (LTR303_ALS_INTEGRATE_TIME_200MS || LTR303_ALS_MEASURE_RATE_1000MS)) != 0) {
        LOG_ERR("Failed to set ALS Measurement Rate");
        return -EIO;
    }

    return 0;
}

int ltr303_init(const struct device *dev)
{
    if (ltr303_chip_init(dev) < 0) {
        return -EINVAL;
    }

    return 0;
}

DEVICE_DT_INST_DEFINE(0, ltr303_init, NULL,
                      &ltr303_drv_data, NULL, POST_KERNEL,
                      CONFIG_SENSOR_INIT_PRIORITY, &ltr303_driver_api);
