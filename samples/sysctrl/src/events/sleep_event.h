/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _SLEEP_EVENT_H_
#define _SLEEP_EVENT_H_

/**
 * @brief Sleep Event
 * @defgroup sleep_event Sleep Event
 * @{
 */

#include "pm/pm.h"
#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sleep_event {
	struct event_header	header;
	struct pm_request_sleep	request;
};

EVENT_TYPE_DECLARE(sleep_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _SLEEP_EVENT_H_ */
