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

#include "led.h"

#define LED_GREEN_NODE	DT_ALIAS(led_green)
#if DT_NODE_HAS_STATUS(LED_GREEN_NODE, okay)
#define LED_GREEN_LABEL	DT_GPIO_LABEL(LED_GREEN_NODE, gpios)
#define LED_GREEN_PIN	DT_GPIO_PIN(LED_GREEN_NODE, gpios)
#define LED_GREEN_FLAGS	DT_GPIO_FLAGS(LED_GREEN_NODE, gpios)
#else
#error "Unsupported board: LED_GREEN devicetree alias not found"
#endif

#define LED_RED_NODE	DT_ALIAS(led_red)
#if DT_NODE_HAS_STATUS(LED_RED_NODE, okay)
#define LED_RED_LABEL	DT_GPIO_LABEL(LED_RED_NODE, gpios)
#define LED_RED_PIN	DT_GPIO_PIN(LED_RED_NODE, gpios)
#define LED_RED_FLAGS	DT_GPIO_FLAGS(LED_RED_NODE, gpios)
#else
#error "Unsupported board: LED_RED devicetree alias not found"
#endif

#define LED_BLUE_NODE	DT_ALIAS(led_blue)
#if DT_NODE_HAS_STATUS(LED_BLUE_NODE, okay)
#define LED_BLUE_LABEL	DT_GPIO_LABEL(LED_BLUE_NODE, gpios)
#define LED_BLUE_PIN	DT_GPIO_PIN(LED_BLUE_NODE, gpios)
#define LED_BLUE_FLAGS	DT_GPIO_FLAGS(LED_BLUE_NODE, gpios)
#else
#error "Unsupported board: LED_BLUE devicetree alias not found"
#endif

LOG_MODULE_REGISTER(led);

#define MAX_LED_EVENTS_IN_FIFO	10

#define THREAD_STACK_SIZE	2048
#define THREAD_PRIORITY		K_PRIO_PREEMPT(14)

K_FIFO_DEFINE(led_fifo);
static K_THREAD_STACK_DEFINE(thread_stack, THREAD_STACK_SIZE);
static struct k_thread thread;


enum led_action {
	LED_ACTION_OFF,
	LED_ACTION_ON,
	LED_ACTION_BLINK_FAST,
	LED_ACTION_BLINK_SLOW,
	LED_ACTION_MAX,
};

struct led_dev {
	const struct device *dev;
	gpio_pin_t pin;
};

struct led_obj {
	void *fifo_reserved; /* 1st word reserved for use by fifo */

	const struct led_dev *led;
	enum led_action action;
	uint32_t value;
	bool pin_state;
};

static volatile uint8_t action_cnt;
static struct led_dev led_dev[LED_MAX];

static int led_init(void)
{
	int ret;

	led_dev[LED_GREEN].dev = device_get_binding(LED_GREEN_LABEL);
	led_dev[LED_GREEN].pin = LED_GREEN_PIN;
	led_dev[LED_RED].dev = device_get_binding(LED_RED_LABEL);
	led_dev[LED_RED].pin = LED_RED_PIN;
	led_dev[LED_BLUE].dev = device_get_binding(LED_BLUE_LABEL);
	led_dev[LED_BLUE].pin = LED_BLUE_PIN;

	ret = gpio_pin_configure(led_dev[LED_GREEN].dev, led_dev[LED_GREEN].pin,
				 GPIO_OUTPUT_INACTIVE | LED_GREEN_FLAGS);
	if (ret) {
		LOG_ERR("%s: LED green init error: [%d]", __FUNCTION__, ret);
		return ret;
	}

	ret = gpio_pin_configure(led_dev[LED_RED].dev, led_dev[LED_RED].pin,
				 GPIO_OUTPUT_INACTIVE | LED_RED_FLAGS);
	if (ret) {
		LOG_ERR("%s: LED red init error: [%d]", __FUNCTION__, ret);
		return ret;
	}

	ret = gpio_pin_configure(led_dev[LED_BLUE].dev, led_dev[LED_BLUE].pin,
				 GPIO_OUTPUT_INACTIVE | LED_BLUE_FLAGS);
	if (ret) {
		LOG_ERR("%s: LED blue init error: [%d]", __FUNCTION__, ret);
		return ret;
	}

	return 0;
}

static void event_poll(struct led_obj **obj)
{
	int key = irq_lock();

	if (k_fifo_is_empty(&led_fifo)) {
		*obj = NULL;
		irq_unlock(key);
		return;
	}

	*obj = k_fifo_get(&led_fifo, K_NO_WAIT);
	irq_unlock(key);

	LOG_INF("%s: action: %d cnt: %d", __FUNCTION__,
		(*obj)->action, (*obj)->value);
}

static void event_start(struct led_obj *obj)
{
	int ret;

	if (obj->action == LED_ACTION_OFF) {
		obj->pin_state = false;
	} else {
		obj->pin_state = true;
	}

	ret = gpio_pin_set(obj->led->dev, obj->led->pin, (int)obj->pin_state);
	if (ret) {
		LOG_WRN("%s: failed: %d", __FUNCTION__, obj->action);
	}
}

static void event_finish(struct led_obj **obj)
{
	int ret;

	ret = gpio_pin_set((*obj)->led->dev, (*obj)->led->pin, false);
	if (ret) {
		LOG_WRN("%s: failed: [%d]", __FUNCTION__, ret);
	}

	int key = irq_lock();
	--action_cnt;
	k_free(*obj);
	*obj = NULL;
	irq_unlock(key);
}

static void led_thread_fn(void)
{
	static struct led_obj *active_obj = NULL;

	while(1) {
		k_sleep(K_MSEC(100));

		if (active_obj == NULL) {
			event_poll(&active_obj);
			if (active_obj) {
				event_start(active_obj);
			}
			continue;
		}

		switch (active_obj->action) {
		case LED_ACTION_OFF:
			event_finish(&active_obj); // led on = false
			break;
		case LED_ACTION_ON:
			if (!k_fifo_is_empty(&led_fifo)) {
				event_finish(&active_obj); // led on == true
			}
			break;
		case LED_ACTION_BLINK_SLOW:
			k_sleep(K_MSEC(400));
			__fallthrough;
		case LED_ACTION_BLINK_FAST:
			if (active_obj->value == LED_BLINK_INFINITE) {
				if (!k_fifo_is_empty(&led_fifo)) {
					active_obj->value = 1;
				}
			}
			active_obj->pin_state = !active_obj->pin_state;
			gpio_pin_set(active_obj->led->dev, active_obj->led->pin,
				     (int)active_obj->pin_state);

			if (!active_obj->pin_state) {
				active_obj->value--;
				if (active_obj->value == 0) {
					event_finish(&active_obj);
				}
			}
			break;
		default:
			event_finish(&active_obj);
			break;
		}
	}
}

static struct led_obj *get_led_obj(void)
{
	int key;

	key = irq_lock();

	if (action_cnt >= MAX_LED_EVENTS_IN_FIFO) {
		irq_unlock(key);
		LOG_WRN("%s: fifo full failed", __FUNCTION__);
		return NULL;
	}

	action_cnt++;
	irq_unlock(key);

	size_t size = sizeof(struct led_obj);
	struct led_obj *ptr = (struct led_obj *)k_malloc(size);

	if (ptr == NULL) {
		LOG_WRN("%s: malloc failed", __FUNCTION__);
		return NULL;
	}

	return ptr;
}

static int led_blink(enum led_color color, uint8_t cnt, enum led_action action)
{
	struct led_obj *ptr;

	if (cnt == 0) {
		return -EINVAL;
	}

	if (color >= LED_MAX) {
		return -EINVAL;
	}

	ptr = get_led_obj();
	if (ptr == NULL) {
		return -ENOMEM;
	}

	ptr->led = &led_dev[color];
	ptr->action = action;
	ptr->value = cnt;
	ptr->pin_state = false;
	k_fifo_put(&led_fifo, ptr);

	return 0;
}

int led_thread_init(void)
{
	int ret = led_init();

	LOG_INF("LED module initialization: [%s]", ret == 0 ? "OK" : "FAILED");
	if (ret != 0) {
		return ret;
	}

	k_thread_create(&thread, thread_stack,
			THREAD_STACK_SIZE,
			(k_thread_entry_t) led_thread_fn,
			NULL, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&thread, "led_thread");

	return 0;
}

int led_set(enum led_color color, bool enable)
{
	struct led_obj *ptr;

	if (color >= LED_MAX) {
		return -EINVAL;
	}

	ptr = get_led_obj();
	if (ptr == NULL) {
		return -ENOMEM;
	}

	ptr->led = &led_dev[color];
	if (enable) {
		ptr->action = LED_ACTION_ON;
	} else {
		ptr->action = LED_ACTION_OFF;
	}
	k_fifo_put(&led_fifo, ptr);

	return 0;
}

int led_blink_fast(enum led_color color, uint8_t cnt)
{
	int ret;

	ret = led_blink(color, cnt, LED_ACTION_BLINK_FAST);

	if (ret) {
		LOG_WRN("%s: failed", __FUNCTION__);
	}

	return ret;
}

int led_blink_slow(enum led_color color, uint8_t cnt)
{
	int ret;

	ret = led_blink(color, cnt, LED_ACTION_BLINK_SLOW);

	if (ret) {
		LOG_WRN("%s: failed", __FUNCTION__);
	}

	return ret;
}
