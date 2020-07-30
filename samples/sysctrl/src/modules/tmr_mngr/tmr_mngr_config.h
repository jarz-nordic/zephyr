/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef TMR_MNGR_CONFIG_H_
#define TMR_MNGR_CONFIG_H_

#include <nrf.h>

#define NRF_ASSERT(a) (void)(a)

#define HW_RTC_LEN_MASK 0x00FFFFFFuL

#define TMR_MNGR_NUM	8

#define TMR_MNGR_AVAL_TIMERS \
	{					\
		0x000000FF,			\
	}

#define TMR_MNGR_LOCK(key)   key = irq_lock()
#define TMR_MNGR_UNLOCK(key) irq_unlock(key)

#define CONFIG_USE_64_EMUL_TIMER 1

#if CONFIG_USE_64_EMUL_TIMER
typedef uint64_t timer_native_t;

static volatile struct {
	uint64_t counter_value;
	uint64_t cc_value;

} timer_64_emul;
#else
typedef uint32_t timer_native_t;
#endif // CONFIG_USE_64_EMUL_TIMER

#endif // TMR_MNGR_CONFIG_H_
