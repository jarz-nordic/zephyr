/*
 * Copyright (c) 2020 Nordic Semuconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @brief Power Control Module: Power Control Dispatcher
 */

#ifndef ZEPHYR_INCLUDE_PM_PCM_DISPATCHER_H_
#define ZEPHYR_INCLUDE_PM_PCM_DISPATCHER_H_

#include "pm/pm.h"

/**
 * @brief Dispatcher event type.
 * Source of dispatcher event.
 */
enum pm_pcm_dispatcher_event {
	PM_PCM_DISPATCHER_EVENT_POWER,
	PM_PCM_DISPATCHER_EVENT_PMIC,
	PM_PCM_DISPATCHER_EVENT_DELAYED,
};

/** @brief Initialize Power Control Dispatcher. */
void pm_pcm_dispatcher_init(void);

/**
 * @brief Pass event to dispatcher.
 *
 * Direct event to be handled by Power Control Dispatcher.
 *
 * @param evt_type Type of event to be handled.
 * @param ctx      Pointer to event to be handled. It will be cast based on @ref evt_type.
 *
 * @return Structure with response.
 */
struct pm_return pm_pcm_dispatcher_event_request(enum pm_pcm_dispatcher_event evt_type,
						 void * ctx);

/**
 * @brief Request a traversal.
 *
 * This function handles power request using traversal algorithm.
 * Depending on request time and previous requests it will try to find
 * an acceptable solution to the given request.
 *
 * @param req Pointer to power request to be handled.
 *
 * @return Structure with response.
 */
struct pm_return pm_pcm_dispatcher_traversal_request(struct pm_request_power *req);

#endif /* ZEPHYR_INCLUDE_PM_PCM_DISPATCHER_H_ */
