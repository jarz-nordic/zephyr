/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "send_event.h"
#include <stdio.h>

static void profile_send_event(struct log_event_buf *buf,
				 const struct event_header *eh)
{
	struct send_event *event = cast_send_event(eh);

	profiler_log_encode_u32(buf, event->size);
	profiler_log_encode_u32(buf, event->endpoint);
}

static int log_send_event(const struct event_header *eh, char *buf,
			    size_t buf_len)
{
	struct send_event *event = cast_send_event(eh);

	return snprintf(buf, buf_len, "size=%d endpoint=%d", event->size, event->endpoint);
}

EVENT_INFO_DEFINE(send_event,
		  ENCODE(),
		  ENCODE(),
		  profile_send_event);

EVENT_TYPE_DEFINE(send_event,
		  true,
		  log_send_event,
		  &send_event_info);
