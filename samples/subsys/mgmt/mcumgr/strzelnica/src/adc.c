/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr.h>
#include <devicetree.h>
#include <drivers/adc.h>
#include <drivers/gpio.h>
#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>

#include "adc.h"

LOG_MODULE_REGISTER(adc);

static int32_t buffer;

struct adc_device {
	const struct device *dev;
	struct adc_channel_cfg cfg;
	struct adc_sequence sequence;

};

static struct adc_device adc = {
	.dev = NULL,

	.cfg.gain = ADC_GAIN_1_4,
	.cfg.reference = ADC_REF_VDD_1_4,
	.cfg.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10),
	.cfg.channel_id = 0,
	.cfg.differential = 0,
	.cfg.input_positive = 1, // AIN0 -> ADC result: motor current
	.cfg.input_negative = 0,

	.sequence.options = NULL,
	.sequence.channels = BIT(0), // channel 0
	.sequence.buffer = &buffer,
	.sequence.buffer_size = sizeof(buffer),
	.sequence.resolution = 12,
};

int adc_init(void)
{
	int rc;

	adc.dev = device_get_binding(DT_LABEL(DT_NODELABEL(adc)));
	if (!adc.dev) {
		LOG_ERR("%s: Cannot get QDEC device", __FUNCTION__);
		return -ENXIO;
	}

	rc = adc_channel_setup(adc.dev, &adc.cfg);
	LOG_INF("channel setup: [%s]", rc != 0 ? "error" : "ok");
	if (rc) {
		return rc;
	}

	rc = adc_read(adc.dev, &adc.sequence);
	LOG_INF("channel read: [%s]", rc != 0 ? "error" : "ok");
	if (rc == 0) {
		LOG_INF(" -> Measured: %d", buffer);
	}

	return rc;
}

int adc_get(int32_t *result)
{
	int rc;

	rc = adc_read(adc.dev, &adc.sequence);
	*result = buffer;

	return rc;
}
