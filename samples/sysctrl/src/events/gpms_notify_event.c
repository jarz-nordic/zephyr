/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "gpms_notify_event.h"


static void profile_gpms_notify_event(struct log_event_buf *buf,
                  const struct event_header *eh)
{
    struct gpms_notify_event *event = cast_gpms_notify_event(eh);

    ARG_UNUSED(event);
    profiler_log_encode_u32(buf, (uint32_t)event->p_msg);
}

static int log_gpms_notify_event(const struct event_header *eh, char *buf,
             size_t buf_len)
{
    struct gpms_notify_event *event = cast_gpms_notify_event(eh);

    return snprintf(buf, buf_len, "Resp=%p", event->p_msg);
}

EVENT_INFO_DEFINE(gpms_notify_event,
          ENCODE(PROFILER_ARG_U8),
          ENCODE("domain"),
          profile_gpms_notify_event);

EVENT_TYPE_DEFINE(gpms_notify_event,
          true,
          log_gpms_notify_event,
          &gpms_notify_event_info);
