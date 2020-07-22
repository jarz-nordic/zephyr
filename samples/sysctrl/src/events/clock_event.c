/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "clock_event.h"


static void profile_clock_event(struct log_event_buf *buf,
                  const struct event_header *eh)
{
    struct clock_event *event = cast_clock_event(eh);

    ARG_UNUSED(event);
    profiler_log_encode_u32(buf, event->p_msg->domain_id);
}

static int log_clock_event(const struct event_header *eh, char *buf,
             size_t buf_len)
{
    struct clock_event *event = cast_clock_event(eh);

    return snprintf(buf, buf_len, "domain=%d", event->p_msg->domain_id);
}

EVENT_INFO_DEFINE(clock_event,
          ENCODE(PROFILER_ARG_U8),
          ENCODE("domain"),
          profile_clock_event);

EVENT_TYPE_DEFINE(clock_event,
          true,
          log_clock_event,
          &clock_event_info);
