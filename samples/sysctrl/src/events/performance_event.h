/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _PERFORMANCE_EVENT_H_
#define _PERFORMANCE_EVENT_H_

/**
 * @brief Performance Event
 * @defgroup performance_event Performance Event
 * @{
 */

#include "pm/pm.h"
#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct performance_event {
	struct event_header		header;
	struct pm_request_performance	request;
};

EVENT_TYPE_DECLARE(performance_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _PERFORMANCE_EVENT_H_ */
