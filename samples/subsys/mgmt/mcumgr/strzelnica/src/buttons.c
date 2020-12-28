/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>

#include "buttons.h"

#define BUTTON_SPEED_NODE	DT_ALIAS(button_speed)
#if DT_NODE_HAS_STATUS(BUTTON_SPEED_NODE, okay)
#define BUTTON_SPEED_LABEL	DT_GPIO_LABEL(BUTTON_SPEED_NODE, gpios)
#define BUTTON_SPEED_PIN	DT_GPIO_PIN(BUTTON_SPEED_NODE, gpios)
#define BUTTON_SPEED_FLAGS	DT_GPIO_FLAGS(BUTTON_SPEED_NODE, gpios)
#else
#error "Unsupported board: BUTTON_SPEED devicetree alias not found"
#endif

#define BUTTON_CALL_NODE	DT_ALIAS(button_call)
#if DT_NODE_HAS_STATUS(BUTTON_CALL_NODE, okay)
#define BUTTON_CALL_LABEL	DT_GPIO_LABEL(BUTTON_CALL_NODE, gpios)
#define BUTTON_CALL_PIN		DT_GPIO_PIN(BUTTON_CALL_NODE, gpios)
#define BUTTON_CALL_FLAGS	DT_GPIO_FLAGS(BUTTON_CALL_NODE, gpios)
#else
#error "Unsupported board: BUTTON_CALL devicetree alias not found"
#endif

LOG_MODULE_REGISTER(button);

enum button_state {
	BUTTON_STATE_IDLE,
	BUTTON_STATE_PRESSED,
	BUTTON_STATE_VALID_SHORT,
	BUTTON_STATE_VALID_LONG,
	BUTTON_STATE_STUCKED,
};

struct button_dev {
	const struct device *dev;
	gpio_pin_t pin;
};

static struct gpio_callback button_speed_cb_data;
static struct gpio_callback button_call_cb_data;
static struct button_dev button_dev[BUTTON_NAME_MAX];
static button_hander_t app_notify_cb;

static void button_speed_pressed(const struct device *dev,
	       			 struct gpio_callback *cb,
		    		 uint32_t pins)
{
	app_notify_cb(BUTTON_NAME_SPEED, BUTTON_EVT_PRESSED_LONG);
}

static void button_call_pressed(const struct device *dev,
	       			 struct gpio_callback *cb,
		    		 uint32_t pins)
{
	app_notify_cb(BUTTON_NAME_CALL, BUTTON_EVT_PRESSED_SHORT);
}

int buttons_init(button_hander_t button_cb)
{
	int ret;

	if (button_cb == NULL) {
		LOG_WRN("%s: missing callback", __FUNCTION__);
		return -EINVAL;
	}
	app_notify_cb = button_cb;

	button_dev[BUTTON_NAME_SPEED].dev = device_get_binding(BUTTON_SPEED_LABEL);
	button_dev[BUTTON_NAME_SPEED].pin = BUTTON_SPEED_PIN;
	button_dev[BUTTON_NAME_CALL].dev = device_get_binding(BUTTON_CALL_LABEL);
	button_dev[BUTTON_NAME_CALL].pin = BUTTON_CALL_PIN;

	ret = gpio_pin_configure(button_dev[BUTTON_NAME_SPEED].dev,
				 button_dev[BUTTON_NAME_SPEED].pin,
				 BUTTON_SPEED_FLAGS);
	if (ret) {
		LOG_ERR("%s: BUTTON SPEED init error: [%d]", __FUNCTION__, ret);
		return ret;
	}

	ret = gpio_pin_configure(button_dev[BUTTON_NAME_CALL].dev,
				 button_dev[BUTTON_NAME_CALL].pin,
				 BUTTON_CALL_FLAGS);
	if (ret) {
		LOG_ERR("%s: BUTTON CALL init error: [%d]", __FUNCTION__, ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure(button_dev[BUTTON_NAME_SPEED].dev,
					   button_dev[BUTTON_NAME_SPEED].pin,
					   GPIO_INT_DISABLE);
	if (ret) {
		LOG_WRN("%s: button speed irq init err: [%d]", __FUNCTION__,
			ret);
		return ret;
	}
	gpio_init_callback(&button_speed_cb_data,
			   button_speed_pressed,
			   BIT(BUTTON_SPEED_PIN));
	gpio_add_callback(button_dev[BUTTON_NAME_SPEED].dev,
			  &button_speed_cb_data);

	ret = gpio_pin_interrupt_configure(button_dev[BUTTON_NAME_CALL].dev,
					   button_dev[BUTTON_NAME_CALL].pin,
					   GPIO_INT_DISABLE);
	if (ret) {
		LOG_WRN("%s: button call irq init err: [%d]", __FUNCTION__,
			ret);
		return ret;
	}
	gpio_init_callback(&button_call_cb_data,
			   button_call_pressed,
			   BIT(BUTTON_CALL_PIN));
	gpio_add_callback(button_dev[BUTTON_NAME_CALL].dev,
			  &button_call_cb_data);

	return 0;
}

/* enable button interrupt */
static int button_enable(enum button_name name, bool enable)
{
	int ret;
	gpio_flags_t flags;

	flags = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;
	ret = gpio_pin_interrupt_configure(button_dev[name].dev,
					   button_dev[name].pin,
					   flags);
	if (ret) {
		LOG_WRN("%s: button [%d] %s failed: %d", __FUNCTION__,
			name, enable ? "enable" : "disable", ret);
	}

	return ret;
}

int buttons_enable(bool enable)
{
	int ret;

	ret = button_enable(BUTTON_NAME_SPEED, enable);
	if (ret) {
		return ret;
	}

	ret = button_enable(BUTTON_NAME_CALL, enable);

	return ret;
}
