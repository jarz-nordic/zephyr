/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include "tmr_mngr_backend.h"

#include <hal/nrf_rtc.h>

#define SAFE_CC_VAL_PLUS 10

#define RTC_NO 0
#define RTC _CONCAT(NRF_RTC, RTC_NO)
#define RTC_IRQn NRFX_IRQ_NUMBER_GET(RTC)

static inline timer_native_t counter_value_get(void)
{
#if CONFIG_USE_64_EMUL_TIMER
	return timer_64_emul.counter_value;
#else
	return RTC->COUNTER;
#endif
}

static inline timer_native_t cc_value_get(void)
{
#if CONFIG_USE_64_EMUL_TIMER
	return timer_64_emul.cc_value;
#else
	return nrf_rtc_cc_get(RTC, 0);
#endif
}

static inline void cc_value_set(timer_native_t cc)
{
#if CONFIG_USE_64_EMUL_TIMER
	timer_64_emul.cc_value = cc;
#else
	nrf_rtc_cc_set(RTC, 0, cc);
#endif
}

int tmr_back_cc_sync_handler(timer_native_t cc_value)
{
	int ret_code = 0;
	timer_native_t cnt = counter_value_get();
	// if cc_value is lower than actual counter then error should be raised
	if ((cc_value < cnt) || ((cc_value - cnt) < SAFE_CC_VAL_PLUS)) {
		ret_code = -EPERM;
	} else {
		cc_value_set(cc_value);
#if !CONFIG_USE_64_EMUL_TIMER
		nrf_rtc_int_enable(RTC, NRF_RTC_INT_COMPARE0_MASK);
		nrf_rtc_event_enable(RTC, NRF_RTC_EVENT_COMPARE_0);
#endif
	}

	// else return zero
	return ret_code;
}

int tmr_back_cc_handler(timer_native_t cc_value)
{
	int ret_code = 0;
	timer_native_t cnt = counter_value_get();
	if ((cc_value < cnt) || ((cc_value - cnt) < 1)) {
		ret_code = -EPERM;
	} else {
		cc_value_set(cc_value);
#if !CONFIG_USE_64_EMUL_TIMER
		nrf_rtc_int_enable(RTC, NRF_RTC_INT_COMPARE0_MASK);
		nrf_rtc_event_enable(RTC, NRF_RTC_EVENT_COMPARE_0);
#endif
	}

	return ret_code;
}

timer_native_t tmr_back_cc_min_val_get_handler(void)
{
	return SAFE_CC_VAL_PLUS;
}

void tmr_back_start_handler(void)
{
}

void tmr_back_stop_handler(void)
{
	nrf_rtc_task_trigger(RTC, NRF_RTC_TASK_STOP);
}

timer_native_t tmr_back_cnt_get_handler(void)
{
	return (timer_native_t)counter_value_get();
}

bool tmr_back_constr_check_handler(timer_native_t cc_value)
{
	bool ret_val = true;
	timer_native_t cnt = counter_value_get();
	if ((cc_value < cnt) || ((cc_value - cnt) <= SAFE_CC_VAL_PLUS)) {
		ret_val = false;
	}
	return ret_val;
}

void RTC_IRQHandler(void)
{
#if CONFIG_USE_64_EMUL_TIMER
	if (nrf_rtc_event_check(RTC, NRF_RTC_EVENT_TICK)) {
		nrf_rtc_event_clear(RTC, NRF_RTC_EVENT_TICK);
		timer_64_emul.counter_value ++;

		if (counter_value_get() >= cc_value_get()) {
			tmr_mngr_back_cc_irq();
		}
	}
#else
	if (nrf_rtc_event_check(RTC, NRF_RTC_EVENT_COMPARE_0)) {
		nrf_rtc_event_clear(RTC, NRF_RTC_EVENT_COMPARE_0);
		tmr_mngr_back_cc_irq();
	}
#endif
}

void tmr_back_init(void)
{
	IRQ_CONNECT(RTC_IRQn, 0, RTC_IRQHandler, 0, 0);
	NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number(RTC), 0);
	NVIC_ClearPendingIRQ(RTC_IRQn);
	nrf_rtc_prescaler_set(RTC, 0);
#if CONFIG_USE_64_EMUL_TIMER
	nrf_rtc_int_enable(RTC, NRF_RTC_INT_TICK_MASK);
	nrf_rtc_event_enable(RTC, NRF_RTC_EVENT_TICK);
#endif
	NRFX_IRQ_ENABLE(RTC_IRQn);
	nrf_rtc_task_trigger(RTC, NRF_RTC_TASK_START);
}
