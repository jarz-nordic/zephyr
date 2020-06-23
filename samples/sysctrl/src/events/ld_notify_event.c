/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "ld_notify_event.h"


static void profile_ld_notify_event(struct log_event_buf *buf,
				    const struct event_header *eh)
{
	struct ld_notify_event *event = cast_ld_notify_event(eh);

	profiler_log_encode_u32(buf, event->msg.domain_id);
}

static int log_ld_notify_event(const struct event_header *eh, char *buf,
			       size_t buf_len)
{
	struct ld_notify_event *event = cast_ld_notify_event(eh);

	return snprintf(buf, buf_len, "dom=%u,size=%u",
			event->msg.domain_id, event->msg.size);
}

EVENT_INFO_DEFINE(ld_notify_event,
		  ENCODE(PROFILER_ARG_U8),
		  ENCODE("domain"),
		  profile_ld_notify_event);

EVENT_TYPE_DEFINE(ld_notify_event,
		  true,
		  log_ld_notify_event,
		  &ld_notify_event_info);
