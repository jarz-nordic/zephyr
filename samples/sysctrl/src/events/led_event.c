/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "led_event.h"


static void profile_led_event(struct log_event_buf *buf,
			      const struct event_header *eh)
{
	struct led_event *event = cast_led_event(eh);

	ARG_UNUSED(event);
	profiler_log_encode_u32(buf, event->domain);
}

static int log_led_event(const struct event_header *eh, char *buf,
			 size_t buf_len)
{
	struct led_event *event = cast_led_event(eh);

	return snprintf(buf, buf_len, "domain=%d", event->domain);
}

EVENT_INFO_DEFINE(led_event,
		  ENCODE(PROFILER_ARG_U8),
		  ENCODE("domain"),
		  profile_led_event);

EVENT_TYPE_DEFINE(led_event,
		  true,
		  log_led_event,
		  &led_event_info);
