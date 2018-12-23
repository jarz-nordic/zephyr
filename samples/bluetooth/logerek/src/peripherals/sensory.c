/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <gpio.h>
#include <sensor.h>
#include <misc/printk.h>
#include <logging/log.h>

#include "sensory.h"
#include "display.h"

LOG_MODULE_REGISTER(app_sensory, LOG_LEVEL_DBG);

#define SENSORY_STACK_SIZE	(1024U)

enum periph_device {
	DEV_IDX_HDC1010,
	DEV_IDX_MMA8652,
	DEV_IDX_APDS9960,
	DEV_IDX_EPD,
	DEV_IDX_NUMOF,
};

static K_THREAD_STACK_DEFINE(sensors_thread_stack, SENSORY_STACK_SIZE);
static struct k_delayed_work temperature_external_timeout;
static k_thread_stack_t *stack = sensors_thread_stack;
static const char *thread_name = "sensory";
struct k_thread sensors_thread;

static s32_t temperature_external = INVALID_SENSOR_VALUE;
static s32_t temperature = INVALID_SENSOR_VALUE;
static s32_t humidity = INVALID_SENSOR_VALUE;

struct device_info {
	struct device *dev;
	char *name;
};

struct led_device_info {
	struct device *dev;
	char *name;
	u32_t pin;
};

static struct device_info dev_info[] = {
	{ NULL, DT_HDC1008_NAME },	/* temp / hum */
	{ NULL, DT_FXOS8700_NAME },	/* accelerometer */
	{ NULL, DT_APDS9960_DRV_NAME },	/* RGB gesture */
	{ NULL, DT_SSD1673_DEV_NAME },	/* epaper */
};

static void timeout_handle(struct k_work *work)
{
	temperature_external = INVALID_SENSOR_VALUE;
}

/* humidity & temperature */
static int get_hdc1010_val(void)
{
	struct sensor_value sensor;

	if (sensor_sample_fetch(dev_info[DEV_IDX_HDC1010].dev)) {
		LOG_ERR("Failed to fetch sample for device %s",
			dev_info[DEV_IDX_HDC1010].name);

		temperature = INVALID_SENSOR_VALUE;
		humidity = INVALID_SENSOR_VALUE;
		return -1;
	}

	if (sensor_channel_get(dev_info[DEV_IDX_HDC1010].dev,
			       SENSOR_CHAN_AMBIENT_TEMP, &sensor)) {
		temperature = INVALID_SENSOR_VALUE;
		LOG_ERR("Invalid Temperature value");
		return -1;
	}
	temperature = sensor.val1 * 10 + sensor.val2 / 100000;

	if (sensor_channel_get(dev_info[DEV_IDX_HDC1010].dev,
			       SENSOR_CHAN_HUMIDITY, &sensor)) {
		humidity = INVALID_SENSOR_VALUE;
		LOG_ERR("Invalid Humidity value");
		return -1;
	}
	humidity = sensor.val1;

	return 0;
}

#if 0
/* xyz accelerometer */
static int get_mma8652_val(struct sensor_value *val)
{
	if (sensor_sample_fetch(dev_info[DEV_IDX_MMA8652].dev)) {
		LOG_ERR("Failed to fetch sample for device %s\n",
		       dev_info[DEV_IDX_MMA8652].name);
		return -1;
	}

	if (sensor_channel_get(dev_info[DEV_IDX_MMA8652].dev,
			       SENSOR_CHAN_ACCEL_XYZ, &val[0])) {
		LOG_ERR("Invalid Accelerometer value");
		return -1;
	}

	return 0;
}

/* RGB Gesture */
static int get_apds9960_val(struct sensor_value *val)
{
	if (sensor_sample_fetch(dev_info[DEV_IDX_APDS9960].dev)) {
		LOG_ERR("Failed to fetch sample for device %s\n",
		       dev_info[DEV_IDX_APDS9960].name);
		return -1;
	}

	if (sensor_channel_get(dev_info[DEV_IDX_APDS9960].dev,
			       SENSOR_CHAN_LIGHT, &val[0])) {
		LOG_ERR("Invalid Light value");
		return -1;
	}

	if (sensor_channel_get(dev_info[DEV_IDX_APDS9960].dev,
			       SENSOR_CHAN_PROX, &val[1])) {
		LOG_ERR("Invalid proximity value");
		return -1;
	}

	return 0;
}
#endif

static void sensors_thread_function(void *arg1, void *arg2, void *arg3)
{
	while (1) {
		get_hdc1010_val();
		display_screen(SCREEN_SENSORS);
		k_sleep(K_MINUTES(1));
	}
}

int sensory_init(void)
{
	k_delayed_work_init(&temperature_external_timeout, timeout_handle);

	for (u8_t i = 0; i < ARRAY_SIZE(dev_info); i++) {
		dev_info[i].dev = device_get_binding(dev_info[i].name);
		if (dev_info[i].dev == NULL) {
			LOG_ERR("Failed to get %s device", dev_info[i].name);
			return -EBUSY;
		}
	}

	k_tid_t tid = k_thread_create(&sensors_thread,
				      stack,
				      SENSORY_STACK_SIZE,
				      sensors_thread_function,
				      NULL,
				      NULL,
				      NULL,
				      K_LOWEST_APPLICATION_THREAD_PRIO,
				      0,
				      0);

	k_thread_name_set(tid, thread_name);

	return 0;
}

int sensory_get_temperature(void)
{
	return temperature;
}

int sensory_get_temperature_external(void)
{
	return temperature_external;
}

void sensory_set_temperature_external(s16_t tmp)
{
	if ((tmp >= -50) && (tmp <= 125)) {
		temperature_external = tmp;
		k_delayed_work_submit(&temperature_external_timeout,
				      K_SECONDS(90));
	}
}

int sensory_get_humidity(void)
{
	return humidity;
}


