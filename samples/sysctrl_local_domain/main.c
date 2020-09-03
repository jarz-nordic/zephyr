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

#include <prism_dispatcher.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MAIN, LOG_LEVEL_INF);

#define DPPI_PIN	23
#define AUX_PIN		11
#define p_reg 		NRF_GPIOTE

#define LOCAL_DOMAIN_TIMER NRF_TIMER0

static volatile uint64_t curr_time;

static int HFCLK_init(struct device *dev)
{
	ARG_UNUSED(dev);
	nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED);
	nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLKSTART);
	while (!nrf_clock_hf_is_running(NRF_CLOCK, NRF_CLOCK_HFCLK_HIGH_ACCURACY)) {};
	return 0;
}
SYS_INIT(HFCLK_init, PRE_KERNEL_1, 0);

static void GPIOTE_IRQHandler(void *arg)
{
	ARG_UNUSED(arg);
	nrf_gpiote_event_clear(NRF_GPIOTE, NRF_GPIOTE_EVENT_IN_0);
	nrf_gpiote_event_disable(NRF_GPIOTE, 0);
}

static inline uint64_t get_counter(void)
{
	nrf_timer_task_trigger(LOCAL_DOMAIN_TIMER, NRF_TIMER_TASK_CAPTURE5);
	return (uint64_t)nrf_timer_cc_get(LOCAL_DOMAIN_TIMER, NRF_TIMER_CC_CHANNEL5);
}

void prism_irq_handler(void)
{
	LOG_INF("IPC IRQ handler invoked.");
}

static void eptx_handler(void)
{
	prism_dispatcher_msg_t msg;

	prism_dispatcher_recv(&msg);
	LOG_INF("[Domain: %d | Ept: %u] handler invoked.", msg.domain_id, msg.ept_id);
	prism_dispatcher_free(&msg);
}

static const prism_dispatcher_ept_t m_epts[] = {
	{ PRISM_DOMAIN_SYSCTRL, 0, eptx_handler },
	{ PRISM_DOMAIN_SYSCTRL, 1, eptx_handler },
	{ PRISM_DOMAIN_SYSCTRL, 2, eptx_handler },
	{ PRISM_DOMAIN_SYSCTRL, 3, eptx_handler },
};

static void timer_channel_init(void)
{
	nrf_gpio_cfg_output(AUX_PIN);
	nrf_timer_event_clear(LOCAL_DOMAIN_TIMER, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_int_enable(LOCAL_DOMAIN_TIMER, TIMER_INTENSET_COMPARE0_Msk);
	nrf_timer_subscribe_set(LOCAL_DOMAIN_TIMER, NRF_TIMER_TASK_START, 0);
	nrf_timer_subscribe_set(LOCAL_DOMAIN_TIMER, NRF_TIMER_TASK_CAPTURE0, 0);
	nrf_timer_bit_width_set(LOCAL_DOMAIN_TIMER, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_frequency_set(LOCAL_DOMAIN_TIMER, NRF_TIMER_FREQ_1MHz);

	nrf_gpiote_event_configure(NRF_GPIOTE, 0, DPPI_PIN, NRF_GPIOTE_POLARITY_LOTOHI);
	nrf_gpiote_publish_set(NRF_GPIOTE, NRF_GPIOTE_EVENT_IN_0, 0);

	nrf_gpiote_event_enable(NRF_GPIOTE, 0);

	nrf_dppi_channels_enable(NRF_DPPIC, 1);

	//IRQ_CONNECT(TIMER0_IRQn, 1, TIMER0_IRQHandler, 0, 0);
	//irq_enable(TIMER0_IRQn);
	nrf_gpio_pin_set(AUX_PIN);
}

static void synchronize_timers(void)
{
	nrf_gpio_pin_clear(AUX_PIN);
	uint64_t req = 0xFFFFFFFF;

	nrf_timer_task_trigger(LOCAL_DOMAIN_TIMER, NRF_TIMER_TASK_STOP);
	nrf_timer_task_trigger(LOCAL_DOMAIN_TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_gpiote_event_enable(NRF_GPIOTE, 0);

	//SyncRequest
	prism_dispatcher_backdoor_xfer(&req);
	curr_time = req;


	while(!nrf_gpiote_event_check(NRF_GPIOTE, NRF_GPIOTE_EVENT_IN_0));
	nrf_gpio_pin_set(AUX_PIN);
	nrf_gpiote_event_disable(NRF_GPIOTE, 0);
	nrf_gpiote_event_clear(NRF_GPIOTE, NRF_GPIOTE_EVENT_IN_0);
	nrf_gpio_pin_set(AUX_PIN);
	LOG_INF("CTR:%u", (uint32_t)curr_time);
}

void main(void)
{
	prism_dispatcher_err_t status;

	status = prism_dispatcher_init(prism_irq_handler, m_epts,
				       ARRAY_SIZE(m_epts));
	if (status != PRISM_DISPATCHER_OK) {
		LOG_ERR("Dispatcher init failed: %d", status);
	}
	timer_channel_init();
	k_sleep(K_MSEC(1000));

	synchronize_timers();

	while(1) {
		k_sleep(K_MSEC(1000));
		synchronize_timers();
	}
}
