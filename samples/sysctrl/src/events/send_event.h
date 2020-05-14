/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _SEND_EVENT_H_
#define _SEND_EVENT_H_

/**
 * @brief Send Event
 * @defgroup send_event Send Event
 * @{
 */

#include "event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct send_event {
	struct event_header header;
	size_t size;
	u8_t   payload[200];
	u32_t  endpoint;
};

EVENT_TYPE_DECLARE(send_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _SEND_EVENT_H_ */
