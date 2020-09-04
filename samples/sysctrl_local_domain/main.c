/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define USE_GPIOTE_NRFX 0
#define USE_GPIOTE_HAL 1
#define USE_TIMER_NRFX 1
#define USE_TIMER_HAL 0

#include <zephyr.h>

#if USE_GPIOTE_NRFX
#include <nrfx_dppi.h>
#include <nrfx_gpiote.h>
#elif USE_GPIOTE_HAL
#include <hal/nrf_dppi.h>
#include <hal/nrf_gpiote.h>
#endif
#include <hal/nrf_timer.h>
#if USE_TIMER_NRFX
#include <nrfx_timer.h>
#endif
#include <hal/nrf_gpio.h>
#include <hal/nrf_clock.h>
#include <device.h>
#include <init.h>

#include <prism_dispatcher.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MAIN, LOG_LEVEL_INF);

#define DPPI_PIN	23
#define AUX_PIN		11

#define LOCAL_DOMAIN_TIMER NRF_TIMER0
typedef uint32_t timer_val_t;
static volatile uint64_t time_base;
static volatile timer_val_t last_sync;
#if USE_TIMER_NRFX
static nrfx_timer_t timer = NRFX_TIMER_INSTANCE(0);
#endif


static int HFCLK_init(struct device *dev)
{
	ARG_UNUSED(dev);
	nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED);
	nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLKSTART);
	while (!nrf_clock_hf_is_running(NRF_CLOCK, NRF_CLOCK_HFCLK_HIGH_ACCURACY)) {};
	return 0;
}
SYS_INIT(HFCLK_init, PRE_KERNEL_1, 0);

#if USE_GPIOTE_NRFX
static volatile bool gpiote_event_occured;
static void GPIOTE_IRQHandler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	ARG_UNUSED(pin);
	ARG_UNUSED(action);
	set_last_sync_value(nrfx_timer_capture(&timer, 1));
	gpiote_event_occured = true;
}
#elif USE_GPIOTE_HAL
static void GPIOTE_IRQHandler(void *arg)
{
	ARG_UNUSED(arg);
	nrf_gpiote_event_clear(NRF_GPIOTE, NRF_GPIOTE_EVENT_IN_0);
	nrf_gpiote_event_disable(NRF_GPIOTE, 0);
}
#endif

static inline void set_last_sync_value(timer_val_t val)
{
	last_sync = val;
}

static inline timer_val_t get_last_sync_value(void)
{
	return last_sync;
}

static inline timer_val_t get_counter(void)
{
#if USE_TIMER_NRFX
	return nrfx_timer_capture(&timer, 0);
#elif USE_TIMER_HAL
	nrf_timer_task_trigger(LOCAL_DOMAIN_TIMER, NRF_TIMER_TASK_CAPTURE0);
	return (uint64_t)nrf_timer_cc_get(LOCAL_DOMAIN_TIMER, NRF_TIMER_CC_CHANNEL0);
#endif
}

static inline uint64_t get_current_time(void)
{
	return (time_base + get_counter() - get_last_sync_value());
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
#if USE_TIMER_NRFX
	nrfx_err_t err;
	nrfx_timer_config_t tmr_config = NRFX_TIMER_DEFAULT_CONFIG;

	tmr_config.mode = NRF_TIMER_MODE_TIMER;
	tmr_config.bit_width = NRF_TIMER_BIT_WIDTH_32;
	tmr_config.frequency = NRF_TIMER_FREQ_1MHz;
	err = nrfx_timer_init(&timer,
		      &tmr_config,
		      NULL);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("Timer already initialized, "
			"switching to software byte counting.");
	}
	//nrf_timer_subscribe_set(timer.p_reg, NRF_TIMER_TASK_START, 0);
	nrf_timer_subscribe_set(timer.p_reg, NRF_TIMER_TASK_CAPTURE0, 0);
	nrfx_timer_clear(&timer);
	nrfx_timer_enable(&timer);

#elif USE_TIMER_HAL
	nrf_timer_event_clear(LOCAL_DOMAIN_TIMER, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_int_enable(LOCAL_DOMAIN_TIMER, TIMER_INTENSET_COMPARE0_Msk);
	nrf_timer_subscribe_set(LOCAL_DOMAIN_TIMER, NRF_TIMER_TASK_START, 0);
	nrf_timer_subscribe_set(LOCAL_DOMAIN_TIMER, NRF_TIMER_TASK_CAPTURE0, 0);
	nrf_timer_bit_width_set(LOCAL_DOMAIN_TIMER, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_frequency_set(LOCAL_DOMAIN_TIMER, NRF_TIMER_FREQ_1MHz);
#endif

#if USE_GPIOTE_NRFX

	/* Initialize GPIOTE (the interrupt priority passed as the parameter
	 * here is ignored, see nrfx_glue.h).
	 */
	err = nrfx_gpiote_init(0);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gpiote_init error: %08x", err);
		return;
	}
	nrfx_gpiote_in_config_t const in_config = {
		.sense = NRF_GPIOTE_POLARITY_LOTOHI,
		.pull = NRF_GPIO_PIN_PULLUP,
		.is_watcher = false,
		.hi_accuracy = true,
		.skip_gpio_setup = false,
	};
	/* Initialize input pin to generate event on high to low transition
	 * (falling edge) and call button_handler()
	 */
	err = nrfx_gpiote_in_init(DPPI_PIN, &in_config, GPIOTE_IRQHandler);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gpiote_in_init error: %08x", err);
		return;
	}
	uint8_t channel;

	err = nrfx_dppi_channel_alloc(&channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_dppi_channel_alloc error: %08x", err);
		return;
	}
	nrf_gpiote_publish_set(
		NRF_GPIOTE,
		nrfx_gpiote_in_event_get(DPPI_PIN),
		channel);
#elif USE_GPIOTE_HAL
	nrf_gpiote_event_configure(NRF_GPIOTE, 0, DPPI_PIN, NRF_GPIOTE_POLARITY_LOTOHI);
	nrf_gpiote_publish_set(NRF_GPIOTE, NRF_GPIOTE_EVENT_IN_0, 0);

	nrf_gpiote_event_enable(NRF_GPIOTE, 0);

	nrf_dppi_channels_enable(NRF_DPPIC, 1);
#endif
	nrf_gpio_pin_set(AUX_PIN);
}

static void synchronize_timers(void)
{
	nrf_gpio_pin_clear(AUX_PIN);
	uint64_t req = 0xFFFFFFFF;
#if USE_TIMER_NRFX
	nrf_gpiote_event_disable(NRF_GPIOTE, 0);
	//doc nrfx_timer_pause(&timer);
	//doc nrfx_timer_clear(&timer);
#elif USE_TIMER_HAL
	//doc nrf_timer_task_trigger(LOCAL_DOMAIN_TIMER, NRF_TIMER_TASK_STOP);
	//doc nrf_timer_task_trigger(LOCAL_DOMAIN_TIMER, NRF_TIMER_TASK_CLEAR);
#endif

#if USE_GPIOTE_NRFX
	nrfx_gpiote_in_event_enable(DPPI_PIN, false);
#elif USE_GPIOTE_HAL
	nrf_gpiote_event_enable(NRF_GPIOTE, 0);
#endif

	//SyncRequest
	prism_dispatcher_backdoor_xfer(&req);
	//We're storing time base from system controller
	time_base = req;

#if USE_GPIOTE_NRFX
	//Waiting for DPPI event (GRTC_COMPARESYNC)
	while(!gpiote_event_occured) {}
	nrfx_gpiote_in_event_disable(DPPI_PIN);
	//TODO inform the Sysctl, that we don't need DPPI any more
	gpiote_event_occured = false;
#elif USE_GPIOTE_HAL
	while(!nrf_gpiote_event_check(NRF_GPIOTE, NRF_GPIOTE_EVENT_IN_0)) {}
	set_last_sync_value(get_counter());
	nrf_gpiote_event_disable(NRF_GPIOTE, 0);
	nrf_gpiote_event_clear(NRF_GPIOTE, NRF_GPIOTE_EVENT_IN_0);
#endif
	nrf_gpio_pin_set(AUX_PIN);
	LOG_INF("ST:%u\r\n%u\r\n%u", (uint32_t)get_current_time(), (uint32_t)get_last_sync_value(), (uint32_t)time_base);
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
	uint64_t ct = get_current_time();
	uint32_t i = 0;

	while(1) {
		k_sleep(K_MSEC(1000));
		LOG_INF("T:%u\r\n%u", (uint32_t)get_current_time(), get_counter());
		if (i < 2) synchronize_timers();
		i++;
		/*ct = get_current_time();
		if (0 == (ct % 32768)) {
			nrf_gpio_pin_toggle(AUX_PIN);
			//k_sleep(K_USEC(10));

		}*/
	}
}
