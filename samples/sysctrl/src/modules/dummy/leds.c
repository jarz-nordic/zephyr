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
#include "ncm.h"

#include <internal/services/nrfs_led.h>

#define LED0 DT_GPIO_PIN(DT_ALIAS(led0), gpios)
#define LED1 DT_GPIO_PIN(DT_ALIAS(led1), gpios)
#define LED2 DT_GPIO_PIN(DT_ALIAS(led2), gpios)
#define LED3 DT_GPIO_PIN(DT_ALIAS(led3), gpios)

static struct device *dev;

static struct {
	uint32_t dummy_service_specific_data;
	struct ncm_ctx ctx;
} led_ctx[1]; // TODO: should be probably defined via Kconfig

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
	// Save request context
	// led_ctx.dummy_service_specific_data = (...);
	ncm_fill(&led_ctx[0].ctx, evt->p_msg);

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

	prism_event_release(evt->p_msg);

	uint8_t dummy_response[] = { 128, 129, 130 };
	void *p_buffer = ncm_alloc(sizeof(dummy_response));
	(void)memcpy(p_buffer, dummy_response, sizeof(dummy_response));
	ncm_notify(&led_ctx[0].ctx, p_buffer, sizeof(dummy_response));
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
