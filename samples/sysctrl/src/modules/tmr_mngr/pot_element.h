/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef POT_ELEMENT_H_
#define POT_ELEMENT_H_

#include <stdlib.h>
#include <stdint.h>
#include "tmr_mngr_internal.h"
#include "tmr_mngr_config.h"

/*
 * @brief Structure that represents one instance of the timer.
 * @note This structure is used to represent leaf of the tree too.
 */
struct pot_element{
	timer_native_t	    next_cc_value;    /**<Number of ticks when the timer expires. */
	timer_native_t	    periodic_value;   /**<Number of ticks of one full timer period. */
	enum tmr_mngr_mode  vrtc_mode;        /**<Work mode of the timer. */
	enum tmr_mngr_state state;            /**<Internal state of the timer. */
	bool		    reload;           /**<Internal flag that is used to reload timer inside interrupt handler. */
	uint32_t	    context;          /**<Context passed to and hold by the timer instance. */
	uint8_t		    timer_id;         /**<Unique ID number of the timer. */
};

#ifdef POT_INCLUDE_COMPARE_FUNCTION
static uint32_t pot_compare_elems(struct pot_element * p1,
				  struct pot_element * p2)
{
	return p1->next_cc_value > p2->next_cc_value ? 0 : 1;
}
#endif

#endif // #define POT_ELEMENT_H_
