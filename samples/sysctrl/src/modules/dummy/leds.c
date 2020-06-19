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
#include "prism_event.h"

#include <nrfs_led.h>

#define LED0 DT_GPIO_PIN(DT_ALIAS(led0), gpios)
#define LED1 DT_GPIO_PIN(DT_ALIAS(led1), gpios)
#define LED2 DT_GPIO_PIN(DT_ALIAS(led2), gpios)
#define LED3 DT_GPIO_PIN(DT_ALIAS(led3), gpios)

static struct device *dev;

static void led_init(void)
{
	dev = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led0), gpios));

	gpio_pin_configure(dev, LED0, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(dev, LED1, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(dev, LED2, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(dev, LED3, GPIO_OUTPUT_ACTIVE);
}

static void led_handle(struct led_event *evt)
{
	nrfs_led_t *p_req = (nrfs_led_t *)evt->p_msg->p_buffer;

	uint8_t pin;

	switch (p_req->data.led_idx) {
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

	switch (p_req->data.op) {
	case NRFS_LED_OP_ON:
		LOG_INF("Switching LED%u ON", p_req->data.led_idx);
		gpio_pin_set(dev, pin, 1);
		break;
	case NRFS_LED_OP_OFF:
		LOG_INF("Switching LED%u OFF", p_req->data.led_idx);
		gpio_pin_set(dev, pin, 0);
		break;
	case NRFS_LED_OP_TOGGLE:
		LOG_INF("Toggling LED%u", p_req->data.led_idx);
		gpio_pin_toggle(dev, pin);
		break;
	}

	struct prism_event *prism_evt = new_prism_event();
	prism_evt->p_msg = evt->p_msg;
	prism_evt->status = PRISM_MSG_STATUS_RX_RELEASED;
	EVENT_SUBMIT(prism_evt);

	uint8_t dummy_response[] = { 1, 2, 3 };

	void *p_buffer = k_malloc(sizeof(dummy_response));
	(void)memcpy(p_buffer, &dummy_response, sizeof(dummy_response));

	nrfs_phy_t *p_msg = k_malloc(sizeof(nrfs_phy_t));
	p_msg->p_buffer = p_buffer;
	p_msg->size = sizeof(dummy_response);
	p_msg->ept_id = evt->p_msg->ept_id;
	p_msg->domain_id = evt->p_msg->domain_id;

	prism_evt = new_prism_event();
	prism_evt->status = PRISM_MSG_STATUS_TX;
	prism_evt->p_msg = p_msg;
	EVENT_SUBMIT(prism_evt);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_status_event(eh)) {
		struct status_event *evt = cast_status_event(eh);
		if (evt->status == STATUS_INIT) {
			led_init();
		}
		return false;
	}

	if (is_led_event(eh)) {
		struct led_event *evt = cast_led_event(eh);
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
