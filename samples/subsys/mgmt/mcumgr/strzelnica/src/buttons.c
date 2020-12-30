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

#define BUTTON_PRESS_SAMPLING_MS	20u
#define BUTTON_DELAY_WORK_SAMPLING	K_MSEC(BUTTON_PRESS_SAMPLING_MS)
#define BUTTON_PRESS_SHORT_MS		20u
#define BUTTON_PRESS_LONG_MS		1000u
#define BUTTON_PRESS_STUCK_MS		90000u

#define BUTTON_PRESS_SHORT_CNT	(BUTTON_PRESS_SHORT_MS/BUTTON_PRESS_SAMPLING_MS)
#define BUTTON_PRESS_LONG_CNT	(BUTTON_PRESS_LONG_MS/BUTTON_PRESS_SAMPLING_MS)
#define BUTTON_PRESS_STUCTK_CNT	(BUTTON_PRESS_STUCK_MS/BUTTON_PRESS_SAMPLING_MS)

LOG_MODULE_REGISTER(button);

enum button_state {
	BUTTON_STATE_IDLE,
	BUTTON_STATE_PRESSED,
	BUTTON_STATE_VALID_SHORT,
	BUTTON_STATE_VALID_LONG,
	BUTTON_STATE_STUCKED,
};

struct button_dev {
	struct k_delayed_work work;

	const struct device *dev;
	gpio_pin_t pin;

	uint16_t cnt;
};

static struct gpio_callback button_speed_cb_data;
static struct gpio_callback button_call_cb_data;
static struct button_dev button_dev[BUTTON_NAME_MAX];
static button_hander_t app_notify_cb;

static void button_speed_pressed(const struct device *dev,
				 struct gpio_callback *cb,
				 uint32_t pins)
{
//	button_enable(BUTTON_NAME_SPEED, false); // disable interrupt
	button_dev[BUTTON_NAME_SPEED].cnt = 0;
	/* start filtering process */
	k_delayed_work_submit(&button_dev[BUTTON_NAME_SPEED].work,
			      BUTTON_DELAY_WORK_SAMPLING);
}

static void button_call_pressed(const struct device *dev,
				struct gpio_callback *cb,
				uint32_t pins)
{
//	button_enable(BUTTON_NAME_CALL, false); // disable interrupt
	LOG_INF("%s", __FUNCTION__);
	button_dev[BUTTON_NAME_CALL].cnt = 0;
	/* start filtering process */
	k_delayed_work_submit(&button_dev[BUTTON_NAME_CALL].work,
			      BUTTON_DELAY_WORK_SAMPLING);
}

/* Function for validating button speed */
static void button_speed_filter(struct k_work *work)
{
	int state;

//	button_enable(BUTTON_NAME_SPEED, true);
	state = gpio_pin_get(button_dev[BUTTON_NAME_SPEED].dev,
			     button_dev[BUTTON_NAME_SPEED].pin);

	/* gpio_pin_get returns true if button is pressed */
	if (state) {
		button_dev[BUTTON_NAME_SPEED].cnt++;
		k_delayed_work_submit(&button_dev[BUTTON_NAME_SPEED].work,
				      BUTTON_DELAY_WORK_SAMPLING);
		return;
	}

	/* In case button is valid or stucked, interrupts must be enabled again
	 * by the main app.
	 */
	if (button_dev[BUTTON_NAME_SPEED].cnt < BUTTON_PRESS_SHORT_CNT) {
		/* Reject due to too short press */
		return;
	}

	button_enable(BUTTON_NAME_SPEED, false);

	if (button_dev[BUTTON_NAME_SPEED].cnt < BUTTON_PRESS_LONG_CNT) {
		app_notify_cb(BUTTON_NAME_SPEED, BUTTON_EVT_PRESSED_SHORT);
	} else if (button_dev[BUTTON_NAME_SPEED].cnt < BUTTON_PRESS_STUCTK_CNT) {
		app_notify_cb(BUTTON_NAME_SPEED, BUTTON_EVT_PRESSED_LONG);
	} else {
		app_notify_cb(BUTTON_NAME_SPEED, BUTTON_EVT_STUCKED);
	}
}

static void button_call_filter(struct k_work *work)
{
	int state;

//	button_enable(BUTTON_NAME_CALL, true);
	state = gpio_pin_get(button_dev[BUTTON_NAME_CALL].dev,
			     button_dev[BUTTON_NAME_CALL].pin);

	/* gpio_pin_get returns true if button is pressed */
	if (state) {
		button_dev[BUTTON_NAME_CALL].cnt++;
		k_delayed_work_submit(&button_dev[BUTTON_NAME_CALL].work,
				      BUTTON_DELAY_WORK_SAMPLING);
		return;
	}

	/* In case button is valid or stucked, interrupts must be enabled again
	 * by the main app.
	 * If the press is shorter than short press interrupts will be resumed
	 */
	if (button_dev[BUTTON_NAME_CALL].cnt < BUTTON_PRESS_SHORT_CNT) {
		/* Reject due to too short press */
		return;
	}

	button_enable(BUTTON_NAME_CALL, false);

	if (button_dev[BUTTON_NAME_CALL].cnt < BUTTON_PRESS_LONG_CNT) {
		app_notify_cb(BUTTON_NAME_CALL, BUTTON_EVT_PRESSED_SHORT);
	} else if (button_dev[BUTTON_NAME_CALL].cnt < BUTTON_PRESS_STUCTK_CNT) {
		app_notify_cb(BUTTON_NAME_CALL, BUTTON_EVT_PRESSED_LONG);
	} else {
		app_notify_cb(BUTTON_NAME_CALL, BUTTON_EVT_STUCKED);
	}
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

	k_delayed_work_init(&button_dev[BUTTON_NAME_SPEED].work,
			    button_speed_filter);

	k_delayed_work_init(&button_dev[BUTTON_NAME_CALL].work,
			    button_call_filter);

	return 0;
}

/* enable button interrupt */
int button_enable(enum button_name name, bool enable)
{
	int ret;
	gpio_flags_t flags;

	if (name >= BUTTON_NAME_MAX) {
		LOG_WRN("%s: bad name:[%d]", __FUNCTION__, name);
		return -EINVAL;
	}

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
