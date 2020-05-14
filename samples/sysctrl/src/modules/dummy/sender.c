/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(MAIN);

#include "send_event.h"


static void send_handle(struct send_event * evt)
{
	LOG_ERR("Sending to endpoint %d, size: %d", evt->endpoint, evt->size);
	LOG_HEXDUMP_ERR(evt->payload, evt->size, "Payload:");
}

static bool event_handler(const struct event_header *eh)
{
	if (is_send_event(eh)) {
		struct send_event * evt = cast_send_event(eh);
		send_handle(evt);
		return true;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(SEND, event_handler);
EVENT_SUBSCRIBE(SEND, send_event);
