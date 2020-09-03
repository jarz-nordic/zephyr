/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <hal/nrf_timer.h>
#include <hal/nrf_dppi.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_gpiote.h>
#include <hal/nrf_rtc.h>

#include <prism_dispatcher.h>
#include <event_manager.h>
#include <status_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MAIN, LOG_LEVEL_INF);


#define AUX_PIN		12
#define DPPI_PIN	9

#define LOCAL_DOMAIN_TIMER_FREQUENCY_Hz	1000000


static volatile bool trg;
static void backdoor_cb(uint64_t *value)
{
	nrf_gpiote_task_enable(NRF_GPIOTE0, 0);
	nrf_gpio_pin_clear(AUX_PIN);
	uint64_t now = (uint64_t)nrf_rtc_counter_get(NRF_RTC1) + 1;
	nrf_rtc_event_clear(NRF_RTC1, NRF_RTC_EVENT_TICK);
	nrf_rtc_event_enable(NRF_RTC1, NRF_RTC_INT_TICK_MASK);
	if (nrf_rtc_event_check(NRF_RTC1, NRF_RTC_EVENT_TICK)) {
		now = (uint64_t)nrf_rtc_counter_get(NRF_RTC1);
		nrf_rtc_event_disable(NRF_RTC1, NRF_RTC_INT_TICK_MASK);
		nrf_rtc_event_clear(NRF_RTC1, NRF_RTC_EVENT_TICK);
	}
	else {
		nrf_rtc_int_enable(NRF_RTC1, NRF_RTC_INT_TICK_MASK);
	}
	*value = ((now * LOCAL_DOMAIN_TIMER_FREQUENCY_Hz) / 32768);
	nrf_gpio_pin_set(AUX_PIN);
	LOG_INF("Time request: %u (now=%u)", (uint32_t)*value, (uint32_t)now - 1);
	trg = true;
}

static void timer_channel_init(void)
{
	nrf_dppi_channels_disable_all(NRF_DPPIC);
	nrf_gpio_cfg_output(AUX_PIN);
	nrf_gpio_pin_set(AUX_PIN);

	nrf_gpiote_task_disable(NRF_GPIOTE0, 0);
	nrf_gpiote_subscribe_set(NRF_GPIOTE0, NRF_GPIOTE_TASK_SET_0, 0);
	nrf_gpiote_task_configure(NRF_GPIOTE0, 0, DPPI_PIN, GPIOTE_CONFIG_POLARITY_LoToHi, NRF_GPIOTE_INITIAL_VALUE_LOW);
	nrf_gpiote_task_enable(NRF_GPIOTE0, 0);

	nrf_dppi_channels_enable(NRF_DPPIC, 1);

	nrf_gpio_pin_mcu_select(11, NRF_GPIO_PIN_MCUSEL_NETWORK);
	nrf_gpio_pin_mcu_select(23, NRF_GPIO_PIN_MCUSEL_NETWORK);
}

void main(void)
{
	if (event_manager_init()) {
		LOG_ERR("Event Manager not initialized");
	} else {
		struct status_event *event = new_status_event();
		event->status = STATUS_INIT;
		EVENT_SUBMIT(event);
	}
	prism_dispatcher_backdoor_register(backdoor_cb);
	timer_channel_init();
	while(1) {
		LOG_INF("CT:%u %x", (uint32_t)z_timer_cycle_get_32(), NRF_RTC1->PUBLISH_TICK);

		while (!trg) k_sleep(K_MSEC(1));
		trg = false;

	}
}
