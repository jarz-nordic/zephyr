/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "temp_event.h"

static void profile_temp_event(struct log_event_buf *buf,
			      const struct event_header *eh)
{
	struct temp_event *event = cast_temp_event(eh);
	profiler_log_encode_u32(buf, event->p_msg->domain_id);
}

static int log_temp_event(const struct event_header *eh, char *buf,
			 size_t buf_len)
{
	struct temp_event *event = cast_temp_event(eh);
	return snprintf(buf, buf_len, "domain=%d", event->p_msg->domain_id);
}

EVENT_INFO_DEFINE(temp_event,
		  ENCODE(PROFILER_ARG_U8),
		  ENCODE("domain"),
		  profile_temp_event);

EVENT_TYPE_DEFINE(temp_event,
		  IS_ENABLED(CONFIG_LOG) ? true : false,
		  IS_ENABLED(CONFIG_LOG) ? log_temp_event : NULL,
		  IS_ENABLED(CONFIG_LOG) ? &temp_event_info : NULL);
