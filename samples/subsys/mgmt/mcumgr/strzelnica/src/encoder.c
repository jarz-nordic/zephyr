/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr.h>
#include <stats/stats.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>

#include "encoder.h"

LOG_MODULE_REGISTER(encoder);

static const struct device *qdec_dev;

static int qdec_init(void)
{
    qdec_dev = device_get_binding(DT_LABEL(DT_NODELABEL(qdec)));
	if (!qdec_dev) {
		LOG_ERR("%s: Cannot get QDEC device", __FUNCTION__);
		return -ENXIO;
	}

	return 0;
}


int encoder_init(void)
{
    qdec_init();
}

int encoder_get(int32_t *data)
{
    int ret;
    struct sensor_value val;

    if (!qdec_dev) {
		LOG_ERR("%s: Cannot get QDEC device", __FUNCTION__);
        return -ENXIO;
    }

    ret = sensor_sample_fetch(qdec_dev);
    if (ret) {
        LOG_WRN("%s: cannot fetch sample. Err: [%d]", __FUNCTION__, ret);
        return ret;
    }

    ret = sensor_channel_get(qdec_dev, SENSOR_CHAN_ROTATION, &val);

    *data = val.val1;

    LOG_INF("%s: samples = %d", __FUNCTION__, *data);

    return ret;
}
