/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(MAIN);

#include "status_event.h"
#include "led_event.h"

#define LED0 DT_GPIO_PIN(DT_ALIAS(led0), gpios)
#define LED1 DT_GPIO_PIN(DT_ALIAS(led1), gpios)
#define LED2 DT_GPIO_PIN(DT_ALIAS(led2), gpios)
#define LED3 DT_GPIO_PIN(DT_ALIAS(led3), gpios)

static struct device * dev;

static void led_init(void)
{
	dev = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led0), gpios));

	gpio_pin_configure(dev, LED0, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(dev, LED1, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(dev, LED2, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(dev, LED3, GPIO_OUTPUT_ACTIVE);
}

static void led_handle(struct led_event * evt)
{
	uint8_t pin;

	switch(evt->domain)
	{
		case 0:
			pin = LED0;
		break;
		case 1:
			pin = LED1;
		break;
		case 2:
			pin = LED2;
		break;
		case 3:
			pin = LED3;
		break;
		default:
			pin = LED0;
		break;
	}

	LOG_INF("Toggling LED%d", evt->domain);
	gpio_pin_toggle(dev, pin);
}


static bool event_handler(const struct event_header *eh)
{
	if (is_status_event(eh)) {
		struct status_event * evt = cast_status_event(eh);
		if (evt->status == STATUS_INIT)
		{
			led_init();
		}
		return false;
	}

	if (is_led_event(eh)) {
		struct led_event * evt = cast_led_event(eh);
		led_handle(evt);
		return true;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(LED, event_handler);
EVENT_SUBSCRIBE(LED, status_event);
EVENT_SUBSCRIBE(LED, led_event);
