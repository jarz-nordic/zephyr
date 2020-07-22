/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "radio_event.h"


static void profile_radio_event(struct log_event_buf *buf,
                  const struct event_header *eh)
{
    struct radio_event *event = cast_radio_event(eh);

    ARG_UNUSED(event);
    profiler_log_encode_u32(buf, event->p_msg->domain_id);
}

static int log_radio_event(const struct event_header *eh, char *buf,
             size_t buf_len)
{
    struct radio_event *event = cast_radio_event(eh);

    return snprintf(buf, buf_len, "domain=%d", event->p_msg->domain_id);
}

EVENT_INFO_DEFINE(radio_event,
          ENCODE(PROFILER_ARG_U8),
          ENCODE("domain"),
          profile_radio_event);

EVENT_TYPE_DEFINE(radio_event,
          true,
          log_radio_event,
          &radio_event_info);
