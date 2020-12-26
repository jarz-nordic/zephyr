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

#define LED_GREEN_NODE	DT_ALIAS(led_green)
#if DT_NODE_HAS_STATUS(LED_GREEN_NODE, okay)
#define LED_GREEN	DT_GPIO_LABEL(LED_GREEN_NODE, gpios)
#define LED_GREEN_PIN	DT_GPIO_PIN(LED_GREEN_NODE, gpios)
#define LED_GREEN_FLAGS	DT_GPIO_FLAGS(LED_GREEN_NODE, gpios)
#else
#error "Unsupported board: LED_GREEN devicetree alias not found"
#endif

#define LED_RED_NODE	DT_ALIAS(led_red)
#if DT_NODE_HAS_STATUS(LED_RED_NODE, okay)
#define LED_RED		DT_GPIO_LABEL(LED_RED_NODE, gpios)
#define LED_RED_PIN	DT_GPIO_PIN(LED_RED_NODE, gpios)
#define LED_RED_FLAGS	DT_GPIO_FLAGS(LED_RED_NODE, gpios)
#else
#error "Unsupported board: LED_RED devicetree alias not found"
#endif

#define LED_BLUE_NODE	DT_ALIAS(led_blue)
#if DT_NODE_HAS_STATUS(LED_BLUE_NODE, okay)
#define LED_BLUE	DT_GPIO_LABEL(LED_BLUE_NODE, gpios)
#define LED_BLUE_PIN	DT_GPIO_PIN(LED_BLUE_NODE, gpios)
#define LED_BLUE_FLAGS	DT_GPIO_FLAGS(LED_BLUE_NODE, gpios)
#else
#error "Unsupported board: LED_BLUE devicetree alias not found"
#endif

LOG_MODULE_REGISTER(led);

#define THREAD_STACK_SIZE	2048
#define THREAD_PRIORITY		K_PRIO_PREEMPT(1)

static K_THREAD_STACK_DEFINE(thread_stack, THREAD_STACK_SIZE);
static struct k_thread thread;

const struct device *dev_green;
const struct device *dev_red;
const struct device *dev_blue;

static inline int led_green_set(bool enable)
{
	return gpio_pin_set(dev_green, LED_GREEN_PIN, (int)enable);
}

static inline int led_red_set(bool enable)
{
	return gpio_pin_set(dev_red, LED_RED_PIN, (int)enable);
}

static inline int led_blue_set(bool enable)
{
	return gpio_pin_set(dev_blue, LED_BLUE_PIN, (int)enable);
}

static int led_init(void)
{
	int ret;

	dev_green = device_get_binding(LED_GREEN);
	dev_red = device_get_binding(LED_RED);
	dev_blue = device_get_binding(LED_BLUE);

	if (!dev_green || !dev_red || !dev_blue) {
		LOG_ERR("%s: LED device not found", __FUNCTION__);
		return -ENXIO;
	}

	ret = gpio_pin_configure(dev_green, LED_GREEN_PIN,
				 GPIO_OUTPUT_ACTIVE | LED_GREEN_FLAGS);
	if (ret) {
		LOG_ERR("%s: green led init error: [%d]", __FUNCTION__, ret);
		return ret;
	}

	ret = gpio_pin_configure(dev_red, LED_RED_PIN,
				 GPIO_OUTPUT_ACTIVE | LED_RED_FLAGS);
	if (ret) {
		LOG_ERR("%s: red led init error: [%d]", __FUNCTION__, ret);
		return ret;
	}

	ret = gpio_pin_configure(dev_green, LED_BLUE_PIN,
				 GPIO_OUTPUT_ACTIVE | LED_BLUE_FLAGS);
	if (ret) {
		LOG_ERR("%s: red led init error: [%d]", __FUNCTION__, ret);
		return ret;
	}


	led_green_set(false);
	led_red_set(false);
	led_blue_set(false);

	return 0;
}

static void led_thread_fn(void)
{
	int i = 1;

	while(1) {
		k_sleep(K_MSEC(1000));
		LOG_INF("led thread executed");
	}
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

