/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "sleep_event.h"

static int log_sleep_event(const struct event_header *eh, char *buf,
			   size_t buf_len)
{
	struct sleep_event *event = cast_sleep_event(eh);

	return snprintf(buf, buf_len, "domain=%d, time=%08x%08x",
			event->request.domain,
			(u32_t)((event->request.time & 0xFFFFFFFF00000000) >> 32),
			(u32_t)(event->request.time & 0x00000000FFFFFFFF));
}

EVENT_INFO_DEFINE(sleep_event,
		  ENCODE(),
		  ENCODE(),
		  NULL);

EVENT_TYPE_DEFINE(sleep_event,
		  true,
		  log_sleep_event,
		  &sleep_event_info);
