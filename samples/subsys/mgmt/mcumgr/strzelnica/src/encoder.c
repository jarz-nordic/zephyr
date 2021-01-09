/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/sensor.h>
#include <shell/shell.h>
#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>

#include "encoder.h"

LOG_MODULE_REGISTER(encoder);

static const struct device *qdec_dev;

static const struct sensor_trigger qdec_trig = {
	.type = SENSOR_TRIG_DATA_READY,
	.chan = SENSOR_CHAN_ROTATION,
};

static struct encoder_result qdec_data;

static int qdec_init(void)
{
	qdec_dev = device_get_binding(DT_LABEL(DT_NODELABEL(qdec)));
	if (!qdec_dev) {
		LOG_ERR("%s: Cannot get QDEC device", __FUNCTION__);
		return -ENXIO;
	}

	return 0;
}

static void data_ready_handler(const struct device *dev,
			       struct sensor_trigger *trig)
{
	struct sensor_value value;

	int err = sensor_channel_get(qdec_dev, SENSOR_CHAN_ROTATION, &value);
	if (err) {
		LOG_ERR("%s: Cannot get sensor value", __FUNCTION__);
		return;
	}

	qdec_data.acc += value.val1;
	qdec_data.accdbl += value.val2;
}

int encoder_init(bool trigger)
{
	struct sensor_value val;
	int ret;

	qdec_data.acc = 0;
	qdec_data.accdbl = 0;

	qdec_init();
	(void)sensor_sample_fetch(qdec_dev);
	(void)sensor_channel_get(qdec_dev, SENSOR_CHAN_ROTATION, &val);

	if (trigger) {
		ret = sensor_trigger_set(qdec_dev,
					 (struct sensor_trigger *)&qdec_trig,
					 data_ready_handler);
	}

	return 0;
}

int encoder_get(struct encoder_result *data)
{
	int ret;
	struct sensor_value val;

	if (!qdec_dev) {
		LOG_ERR("%s: Cannot get QDEC device", __FUNCTION__);
		return -ENXIO;
	}

	ret = sensor_sample_fetch(qdec_dev);
	if (ret) {
		LOG_WRN("%s: cannot fetch sample. Err: [%d]", __FUNCTION__,
			ret);
		return ret;
	}

	ret = sensor_channel_get(qdec_dev, SENSOR_CHAN_ROTATION, &val);

	int key = irq_lock();
	qdec_data.acc += val.val1;
	qdec_data.accdbl += val.val2;
	irq_unlock(key);

	data->acc = qdec_data.acc;
	data->accdbl = qdec_data.accdbl;

	qdec_data.acc = 0;
	qdec_data.accdbl = 0;

	return ret;
}

static int cmd_enc_acc(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "acc = %d | accdbl = %d",
		    qdec_data.acc, qdec_data.accdbl);

	return 0;
}

static int cmd_enc_fetch(const struct shell *shell, size_t argc, char **argv)
{
	struct sensor_value val;

	(void)sensor_sample_fetch(qdec_dev);
	(void)sensor_channel_get(qdec_dev, SENSOR_CHAN_ROTATION, &val);

	shell_print(shell, "acc = %d | accdbl = %d", val.val1, val.val2);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_encoder,
	SHELL_CMD(acc, NULL, "Hexdump params command.", cmd_enc_acc),
	SHELL_CMD(fetch, NULL, "Hexdump params command.", cmd_enc_fetch),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(encoder, &sub_encoder, NULL, NULL);
