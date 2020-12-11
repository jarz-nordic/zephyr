/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _STATUS_EVENT_H_
#define _STATUS_EVENT_H_

/**
 * @brief Status Event
 * @defgroup status_event Status Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

enum device_status {
	STATUS_INIT,
};

struct status_event {
	struct event_header header;
	enum device_status status;
};

bool is_status_event(const struct event_header *eh);
struct status_event *cast_status_event(const struct event_header *eh);
#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _STATUS_EVENT_H_ */
