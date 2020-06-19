/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "prism_event.h"


static void profile_prism_event(struct log_event_buf *buf,
				const struct event_header *eh)
{
	struct prism_event *event = cast_prism_event(eh);

	profiler_log_encode_u32(buf, event->p_msg->domain_id);
}

static int log_prism_event(const struct event_header *eh, char *buf,
			   size_t buf_len)
{
	struct prism_event *event = cast_prism_event(eh);

	return snprintf(buf, buf_len, "dom=%u,size=%u,sts=%d",
			event->p_msg->domain_id, event->p_msg->size, event->status);
}

EVENT_INFO_DEFINE(prism_event,
		  ENCODE(PROFILER_ARG_U8),
		  ENCODE("domain"),
		  profile_prism_event);

EVENT_TYPE_DEFINE(prism_event,
		  true,
		  log_prism_event,
		  &prism_event_info);
