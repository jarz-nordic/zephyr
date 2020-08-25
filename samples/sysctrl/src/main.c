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

#define WAIT_TIME_us	4

#define DPPI_PIN	23
#define OUTPUT_PIN	9
#define AUX_PIN		12

#define SYSCTL_TIMER	NRF_TIMER1

void TIMER1_IRQHandler(void *arg)
{
	nrf_gpio_pin_clear(AUX_PIN);
	if (nrf_timer_event_check(SYSCTL_TIMER, NRF_TIMER_EVENT_COMPARE0)) {
		nrf_timer_event_clear(SYSCTL_TIMER, NRF_TIMER_EVENT_COMPARE0);
	}
	nrf_gpio_pin_set(AUX_PIN);
}

static void backdoor_cb(uint64_t *value)
{
	extern uint32_t z_timer_cycle_get_32(void);
	nrf_gpio_pin_clear(AUX_PIN);
	nrf_gpiote_task_enable(NRF_GPIOTE0, 0);
	nrf_timer_event_clear(SYSCTL_TIMER, NRF_TIMER_EVENT_COMPARE0);
	uint32_t now = z_timer_cycle_get_32();
	*value = (uint32_t)(((uint64_t)z_timer_cycle_get_32() * 1000000) / 32768) + WAIT_TIME_us + 1;
	nrf_timer_task_trigger(SYSCTL_TIMER, NRF_TIMER_TASK_START);
	LOG_INF("Time request: %u (now=%u)", (uint32_t)value, now);
	nrf_gpio_pin_set(AUX_PIN);
}

static void timer_channel_init(void)
{
	nrf_gpio_cfg_output(AUX_PIN);
	nrf_gpio_pin_set(AUX_PIN);
	nrf_timer_task_trigger(SYSCTL_TIMER, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(SYSCTL_TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_timer_event_clear(SYSCTL_TIMER, NRF_TIMER_EVENT_COMPARE0);
	IRQ_CONNECT(TIMER1_IRQn, 1, TIMER1_IRQHandler, 0, 0);
	irq_enable(TIMER1_IRQn);
	nrf_timer_frequency_set(SYSCTL_TIMER, NRF_TIMER_FREQ_1MHz);
	nrf_timer_cc_set(SYSCTL_TIMER, NRF_TIMER_CC_CHANNEL0, WAIT_TIME_us);
	nrf_timer_bit_width_set(SYSCTL_TIMER, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_int_enable(SYSCTL_TIMER, TIMER_INTENSET_COMPARE0_Msk);
	nrf_timer_shorts_enable(SYSCTL_TIMER, 0x0100);
	nrf_timer_shorts_enable(SYSCTL_TIMER, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK);
	nrf_timer_publish_set(SYSCTL_TIMER, NRF_TIMER_EVENT_COMPARE0, 0);

	nrf_gpiote_task_configure(NRF_GPIOTE0, 0, OUTPUT_PIN, GPIOTE_CONFIG_POLARITY_LoToHi, NRF_GPIOTE_INITIAL_VALUE_LOW);
	nrf_gpiote_subscribe_set(NRF_GPIOTE0, NRF_GPIOTE_TASK_SET_0, 0);
	nrf_timer_task_trigger(SYSCTL_TIMER, NRF_TIMER_TASK_CLEAR);

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
	LOG_INF("P: %x", *((uint32_t*)0x5000540C));
	while(1) {
		LOG_INF("CT:%u", (uint32_t)z_timer_cycle_get_32());
		k_sleep(K_MSEC(1000));


	}
}
