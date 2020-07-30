/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdlib.h>
#include <stdint.h>
#include "tmr_mngr.h"
#include "tmr_mngr_backend.h"
#include "tmr_mngr_internal.h"
#include "tmr_mngr_config.h"
#include <pot.h>
#include <nrf_bitmask.h>

/** @brief VRTC instances covered by tree element. */
static struct pot_element vrtc_instances[TMR_MNGR_NUM];

/** @brief Pointers that are used for tree. */
static struct pot_element * p_elements[TMR_MNGR_NUM];

/** @brief Applicaion/service handler. */
static tmr_mngr_handler_t p_app_handler;

static const uint8_t available_timers[((TMR_MNGR_NUM) + 7) / 8] =
		TMR_MNGR_AVAL_TIMERS;

static uint8_t allocated_timers[((TMR_MNGR_NUM) + 7) / 8];

static int tmr_mngr_alloc(void)
{
	int retval = -ENOMEM;
	uint32_t key;

	TMR_MNGR_LOCK(key);
	for (uint32_t i = 0; i < TMR_MNGR_NUM; ++i) {
		if (nrf_bitmask_bit_is_set(i, available_timers)
			&& (!nrf_bitmask_bit_is_set(i, allocated_timers))) {
			nrf_bitmask_bit_set(i, allocated_timers);
			retval = (int)i;
			break;
		}
	}
	TMR_MNGR_UNLOCK(key);
	return retval;
}

static void tmr_mngr_free(int instance)
{
	if (nrf_bitmask_bit_is_set(instance, available_timers)
		&& (nrf_bitmask_bit_is_set(instance, allocated_timers))) {
		nrf_bitmask_bit_clear(instance, allocated_timers);
	}
}

static inline void check_vrtc_mode(struct pot_element *elem)
{
	// Check which type it is:
	switch (elem->vrtc_mode) {
		case TMR_MNGR_MODE_PERIODIC:
			elem->next_cc_value += elem->periodic_value;
			elem->reload = true;
			break;
		case TMR_MNGR_MODE_ONE_SHOT_FORCE:
			/* fall-through */
			/* no break */
		case TMR_MNGR_MODE_ONE_SHOT:
			// Freeing the timer
			tmr_mngr_free(elem->timer_id);
			break;
	}
}

void tmr_mngr_back_cc_irq(void)
{
	bool	 once_again = true;
	bool	 current_process = true;
	timer_native_t curr_time;

	while (once_again) {
		while (current_process) {
			// Get currently processed element:
			struct pot_element *p_pop_element = pot_pop();

			if (p_pop_element->state == TMR_MNGR_STATE_ARMED) {
				p_app_handler(p_pop_element->timer_id,
					      p_pop_element->context);
				check_vrtc_mode(p_pop_element);
			} else {// TMR_MNGR_STATE_STOPPED
				// Timer is marked to remove:
				tmr_mngr_free(p_pop_element->timer_id);
			}

			// get new one:
			struct pot_element * p_new_element = pot_get();

			// get current time:
			curr_time = tmr_back_cnt_get_handler();

			// is it already expired?
			if ((!p_new_element) ||
			    (curr_time < p_new_element->next_cc_value)) {
				// Everything is processed. Now reload!
				current_process = false;
			}
		}

		// Load again the timers
		for (uint32_t i = 0; i < TMR_MNGR_NUM; ++i) {
			if (vrtc_instances[i].reload) {
				vrtc_instances[i].reload = false;
				pot_push(&vrtc_instances[i]);
			}
		}

		// Check if something timedout in a meantime:

		// get new one:
		struct pot_element * p_new_element = pot_get();

		// set new CC
		if (p_new_element) {
			// Ofc if it is possible
			if (tmr_back_cc_handler(p_new_element->next_cc_value)
			    == -EPERM) {
				// No possible. Okay, run once again.
				current_process = true;
			} else {
				once_again = false;
			}
		} else {
			once_again = false;
		}
	}
}

int tmr_mngr_init(tmr_mngr_handler_t p_handler)
{
	if (p_handler == NULL) {
		return -EPERM;
	}

	for (uint32_t i = 0; i < sizeof(allocated_timers); ++i) {
		allocated_timers[i] = 0;
	}
	pot_init(p_elements, TMR_MNGR_NUM);
	for (uint32_t i = 0; i < TMR_MNGR_NUM; ++i) {
		vrtc_instances[i].state = TMR_MNGR_STATE_IDLE;
		vrtc_instances[i].timer_id = i;
	}

	p_app_handler = p_handler;
	tmr_back_init();

	return 0;
}

static int vrtc_reconfig(uint8_t instance, enum tmr_mngr_mode mode,
			 timer_native_t value)
{
	bool force = false;
	int ret_code = 0;

	vrtc_instances[instance].vrtc_mode = mode;

	// Get current root element
	struct pot_element * p_curr_elem = pot_get();

	// Check whether we can use this value:
	switch (mode) {
		case TMR_MNGR_MODE_PERIODIC:
			vrtc_instances[instance].periodic_value = value;
			vrtc_instances[instance].next_cc_value =
					tmr_back_cnt_get_handler() + value;
			break;
		case TMR_MNGR_MODE_ONE_SHOT_FORCE:
			force = true;
			/* fall-through */
			/* no break */
		case TMR_MNGR_MODE_ONE_SHOT:
			vrtc_instances[instance].next_cc_value = value;
			break;
	}

	if (!tmr_back_constr_check_handler(vrtc_instances[instance].next_cc_value)) {
		// This value cannot be set properly...
		// but...
		if (force) {
			vrtc_instances[instance].next_cc_value +=
					tmr_back_cc_min_val_get_handler();
		} else {
			// Anyway... There is no possibility to help
			return -EFAULT;
		}
	}

	// CC needs to be set
	// New instance must be add to tree:
	pot_push(&vrtc_instances[instance]);

	// CC can be set in the rtc. Maybe we should set new CC?
	// Check
	struct pot_element * p_new_elem = pot_get();
	if (p_curr_elem != p_new_elem) {
		// If there's different element -> set new CC
		ret_code = tmr_back_cc_sync_handler(p_new_elem->next_cc_value);
	}

	return ret_code;
}

int tmr_mngr_start(enum tmr_mngr_mode mode, timer_native_t value, uint32_t context)
{
	int ret_code = 0;
	int instance = tmr_mngr_alloc();
	uint32_t key;

	if (instance < 0) {
		return -EPERM;
	}

	// Setup context
	vrtc_instances[instance].context = context;

	TMR_MNGR_LOCK(key);

	// Configure vrtc
	ret_code = vrtc_reconfig(instance, mode, value);

	if (ret_code == 0) {
		vrtc_instances[instance].state = TMR_MNGR_STATE_ARMED;
		ret_code = instance;
	} else {
		vrtc_instances[instance].context = 0;
		tmr_mngr_free(instance);
	}
	TMR_MNGR_UNLOCK(key);

	return ret_code;
}

int tmr_mngr_stop(uint8_t instance)
{
	int ret_code = 0;
	uint32_t key;

	if (vrtc_instances[instance].state != TMR_MNGR_STATE_ARMED) {
		return -EPERM;
	}

	TMR_MNGR_LOCK(key);
	if (!tmr_back_constr_check_handler(vrtc_instances[instance].next_cc_value)) {
		// Value is too close to reconfigure...
		// IRQ will handle it
		vrtc_instances[instance].state = TMR_MNGR_STATE_STOPPED;
		ret_code = -EINPROGRESS;
	} else {
		// Ok we have some time. Trying to reconfigure.
		if (pot_remove(&vrtc_instances[instance]) != 0) {
			// Well... element must be right here in the tree. If not maybe... context
			// with higher priority removed it? Anyway. This situation is really errornous
			// and it is covered by the assert.
			NRF_ASSERT(0);
			ret_code = -EFAULT;
		}
		// And disable ticking
		vrtc_instances[instance].state = TMR_MNGR_STATE_IDLE;
		tmr_mngr_free(instance);
	}
	TMR_MNGR_UNLOCK(key);

	return ret_code;
}

timer_native_t tmr_mngr_cnt_get(void)
{
	return tmr_back_cnt_get_handler();
}

