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

#include <logging/log.h>
LOG_MODULE_REGISTER(nrfx_sample, LOG_LEVEL_INF);


#define INPUT_PIN	DT_GPIO_PIN(DT_ALIAS(sw0), gpios)
#define OUTPUT_PIN	10

#define SYSCTL_TIMER	NRF_TIMER0
//#define LOCAL_DOMAIN_TIMER 0x4100c000

void TIMER0_IRQHandler(void *arg)
{
	nrf_gpio_pin_clear(OUTPUT_PIN);
	if (nrf_timer_event_check(SYSCTL_TIMER, NRF_TIMER_EVENT_COMPARE0)) {
		nrf_timer_event_clear(SYSCTL_TIMER, NRF_TIMER_EVENT_COMPARE0);
	}
	nrf_gpio_pin_set(OUTPUT_PIN);
}



void main(void)
{
	IRQ_CONNECT(TIMER0_IRQn, 1, TIMER0_IRQHandler, 0, 0);
	irq_enable(TIMER0_IRQn);
	nrf_timer_cc_set(SYSCTL_TIMER, NRF_TIMER_CC_CHANNEL0, 10000000);
	nrf_timer_event_clear(SYSCTL_TIMER, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_int_enable(SYSCTL_TIMER, TIMER_INTENSET_COMPARE0_Msk);
	nrf_timer_publish_set(SYSCTL_TIMER, NRF_TIMER_EVENT_COMPARE0, 0);

	nrf_gpiote_task_configure(NRF_GPIOTE0, 0, OUTPUT_PIN, (nrf_gpiote_polarity_t) GPIOTE_CONFIG_POLARITY_None, NRF_GPIOTE_INITIAL_VALUE_LOW);
	nrf_gpiote_task_enable(NRF_GPIOTE0, 0);
	nrf_gpiote_subscribe_set(NRF_GPIOTE0, NRF_GPIOTE_TASK_SET_0, 0);

	nrf_dppi_channels_enable(NRF_DPPIC, 1);

	nrf_gpio_pin_mcu_select(11, NRF_GPIO_PIN_MCUSEL_NETWORK);
	nrf_gpio_pin_mcu_select(23, NRF_GPIO_PIN_MCUSEL_NETWORK);

	nrf_timer_task_trigger(SYSCTL_TIMER, NRF_TIMER_TASK_START);
	LOG_INF("s: %p", NRF_GPIOTE0);
	while(1) {
		k_sleep(K_MSEC(1000));


	}
}
