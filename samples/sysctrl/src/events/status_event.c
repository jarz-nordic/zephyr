/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "status_event.h"
#include <stdio.h>

static void profile_status_event(struct log_event_buf *buf,
				 const struct event_header *eh)
{
	struct status_event *event = cast_status_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->status);
}

static int log_status_event(const struct event_header *eh, char *buf,
			    size_t buf_len)
{
	struct status_event *event = cast_status_event(eh);

	return snprintf(buf, buf_len, "status=%d", event->status);
}

EVENT_INFO_DEFINE(status_event,
		  ENCODE(PROFILER_ARG_U8),
		  ENCODE("status"),
		  profile_status_event);

EVENT_TYPE_DEFINE(status_event,
		  true,
		  log_status_event,
		  &status_event_info);
