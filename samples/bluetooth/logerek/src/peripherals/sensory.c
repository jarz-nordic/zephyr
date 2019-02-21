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
#include "led.h"

LOG_MODULE_REGISTER(app_sensory, LOG_LEVEL_INF);

#define SENSORY_STACK_SIZE	(2048U)

struct device_info {
	struct device *dev;
	char *name;
};

enum periph_device {
	DEV_IDX_HDC1010,
	DEV_IDX_MMA8652,
	DEV_IDX_APDS9960,
	DEV_IDX_EPD,
	DEV_IDX_NUMOF,
};

static struct device_info dev_info[] = {
	{ NULL, DT_TI_HDC1010_0_LABEL },
	{ NULL, DT_NXP_MMA8652FC_0_LABEL },
	{ NULL, DT_AVAGO_APDS9960_0_LABEL },
	{ NULL, DT_SOLOMON_SSD1673FB_0_LABEL }
};
static struct device *power_pin;

static K_THREAD_STACK_DEFINE(sensors_thread_stack, SENSORY_STACK_SIZE);
static struct k_delayed_work temperature_external_timeout;
static k_thread_stack_t *stack = sensors_thread_stack;
static const char *thread_name = "sensory";
struct k_thread sensors_thread;

static s32_t temperature_external = INVALID_SENSOR_VALUE;
static s32_t temperature = INVALID_SENSOR_VALUE;
static s32_t humidity = INVALID_SENSOR_VALUE;

static inline void power_sensors(bool state);

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
		LOG_DBG("Sensors thread tick");
		/* power sensors and display */
		power_sensors(true);
		get_hdc1010_val();
		display_screen(SCREEN_SENSORS);
		power_sensors(false);
		/* switch off sensors and display */
//		k_sleep(K_MINUTES(1));
		k_sleep(K_SECONDS(5));
	}
}

int sensory_init(void)
{
	int ret;

	k_delayed_work_init(&temperature_external_timeout, timeout_handle);

	for (u8_t i = 0; i < ARRAY_SIZE(dev_info); i++) {
		dev_info[i].dev = device_get_binding(dev_info[i].name);
		if (dev_info[i].dev == NULL) {
			LOG_ERR("Failed to get %s device", dev_info[i].name);
			return -EBUSY;
		}
	}

	power_pin = device_get_binding(DT_GPIO_KEYS_V_SENS_GPIO_CONTROLLER);

	if (!power_pin) {
		LOG_ERR("Failed to get %s device", DT_GPIO_KEYS_SWITCH_0_LABEL);
	}
	ret = gpio_pin_configure(power_pin, DT_GPIO_KEYS_SWITCH_0_GPIO_PIN,
				 (GPIO_DIR_OUT));
	if (ret) {
		LOG_ERR("Error configuring power PIN");
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
				      4000);

	k_thread_name_set(tid, thread_name);

	return 0;
}

static inline void power_sensors(bool state)
{
	u32_t val = state ? 0 : 1;

	gpio_pin_write(power_pin, DT_GPIO_KEYS_SWITCH_0_GPIO_PIN, val);
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
	if ((tmp >= -500) && (tmp <= 1250)) {
		temperature_external = tmp;
		k_delayed_work_submit(&temperature_external_timeout,
				      K_SECONDS(90));
	}
}

int sensory_get_humidity(void)
{
	return humidity;
}

