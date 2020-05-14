/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "performance_event.h"

static int log_performance_event(const struct event_header *eh, char *buf,
				 size_t buf_len)
{
	struct performance_event *event = cast_performance_event(eh);

	return snprintf(buf, buf_len, "d=%d r=%d m=%d time=%08x%08x",
			event->request.domain,
			event->request.resource,
			event->request.mode,
			(u32_t)((event->request.time & 0xFFFFFFFF00000000) >> 32),
			(u32_t)(event->request.time & 0x00000000FFFFFFFF));
}

EVENT_INFO_DEFINE(performance_event,
		  ENCODE(),
		  ENCODE(),
		  NULL);

EVENT_TYPE_DEFINE(performance_event,
		  true,
		  log_performance_event,
		  &performance_event_info);
