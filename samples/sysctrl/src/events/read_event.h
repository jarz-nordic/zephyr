/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _READ_EVENT_H_
#define _READ_EVENT_H_

/**
 * @brief Read Event
 * @defgroup read_event Read Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct read_event {
	struct event_header header;
};

EVENT_TYPE_DECLARE(read_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _READ_EVENT_H_ */
