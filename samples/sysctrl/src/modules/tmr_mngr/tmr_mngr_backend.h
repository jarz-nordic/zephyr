/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef TMR_MNGR_BACKEND_H_
#define TMR_MNGR_BACKEND_H_

#include <stdint.h>
#include <stdbool.h>
#include "tmr_mngr_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tmr_mngr Virtual RTC service module.
 * @{
 * @ingroup nrf_services
 * @brief Implementation of standard in and out functions for stdio library.
 */

/** @brief Function for initializing timer peripheral. */
void tmr_back_init(void);

/**
 * @brief Function for setting up compare channel value outside the interrupt
 * 	  routine.
 *
 * @retval 0      Compare channel was set up successfully.
 * @retval -EPERM Value is too close the counter or value already expired.
 */
int tmr_back_cc_sync_handler(timer_native_t cc_value);

/**
 * @brief Function for setting up compare channel value inside the interrupt
 * 	  routine.
 *
 * @retval 0      Compare channel was set up successfully.
 * @retval -EPERM Value is too close the counter or value already expired.
 */
int tmr_back_cc_handler(timer_native_t cc_value);

/**
 * @brief Function for getting minimum supported value that can beset in the
 * 	  compare channel value in compare to counter value.
 *
 * @return Minimum supported value.
 */
timer_native_t tmr_back_cc_min_val_get_handler(void);

/** @brief Function for starting hardware timer. */
void tmr_back_start_handler(void);

/** @brief Function for stopping hardware timer. */
void tmr_back_stop_handler(void);

/**
 * @brief Function for getting current counter value.
 *
 * @return Current counter value.
 */
timer_native_t tmr_back_cnt_get_handler(void);

/**
 * @brief Function for checking constraints of the @ref cc_value in compare to
 * 	  counter of the timer.
 *
 * @retval true  Value can be used to setup new timer.
 * @retval false Value is too close to the counter of the timer.
 */
bool tmr_back_constr_check_handler(timer_native_t cc_value);

/** @brief Function that must be implemented by developer to integrate timer
 * 	   manager with application. */
void tmr_mngr_back_cc_irq(void);


/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif // TMR_MNGR_BACKEND_H_
