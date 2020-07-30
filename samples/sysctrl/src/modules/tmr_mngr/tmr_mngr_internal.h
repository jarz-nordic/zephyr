/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef TMR_MNGR_INTERNAL_H_
#define TMR_MNGR_INTERNAL_H_

#include <stdbool.h>
#include <stdint.h>
#include "tmr_mngr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tmr_mngr Virtual RTC service module.
 * @{
 * @ingroup nrf_services
 * @brief Implementation of standard in and out functions for stdio library.
 */

enum tmr_mngr_state{
	TMR_MNGR_STATE_IDLE,	//< VRTC is not started.
	TMR_MNGR_STATE_STOPPED, //< VRTC is started and armed in the tree.
	TMR_MNGR_STATE_ARMED,   //< VRTC is started and armed in the tree.
};


/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif // TMR_MNGR_H_
