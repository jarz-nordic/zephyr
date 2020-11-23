/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "gpms_event.h"

static void profile_gpms_event(struct log_event_buf *buf,
                  const struct event_header *eh)
{
    struct gpms_event *event = cast_gpms_event(eh);

    ARG_UNUSED(event);
    profiler_log_encode_u32(buf, event->p_msg->domain_id);
}

static int log_gpms_event(const struct event_header *eh, char *buf,
             size_t buf_len)
{
    struct gpms_event *event = cast_gpms_event(eh);

    return snprintf(buf, buf_len, "domain=%d", event->p_msg->domain_id);
}

EVENT_INFO_DEFINE(gpms_event,
          ENCODE(PROFILER_ARG_U8),
          ENCODE("domain"),
          profile_gpms_event);

EVENT_TYPE_DEFINE(gpms_event,
          IS_ENABLED(CONFIG_LOG) ? true : false,
          IS_ENABLED(CONFIG_LOG) ? log_gpms_event : NULL,
          IS_ENABLED(CONFIG_LOG) ? &gpms_event_info : NULL);

static void profile_gpms_response_event(struct log_event_buf *buf,
                  const struct event_header *eh)
{
    struct gpms_response_event *event = cast_gpms_response_event(eh);

    ARG_UNUSED(event);
    profiler_log_encode_u32(buf, (uint32_t)event->p_msg);
}

static int log_gpms_response_event(const struct event_header *eh, char *buf,
             size_t buf_len)
{
    struct gpms_response_event *event = cast_gpms_response_event(eh);

    return snprintf(buf, buf_len, "Resp=%p", event->p_msg);
}

EVENT_INFO_DEFINE(gpms_response_event,
          ENCODE(PROFILER_ARG_U8),
          ENCODE("domain"),
          profile_gpms_response_event);

EVENT_TYPE_DEFINE(gpms_response_event,
          IS_ENABLED(CONFIG_LOG) ? true : false,
          IS_ENABLED(CONFIG_LOG) ? log_gpms_response_event : NULL,
          IS_ENABLED(CONFIG_LOG) ? &gpms_response_event_info : NULL);


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
          IS_ENABLED(CONFIG_LOG) ? true : false,
          IS_ENABLED(CONFIG_LOG) ? log_gpms_notify_event : NULL,
          IS_ENABLED(CONFIG_LOG) ? &gpms_notify_event_info : NULL);

