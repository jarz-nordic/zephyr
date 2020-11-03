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
          IS_ENABLED(CONFIG_LOG) ? true : false,
          IS_ENABLED(CONFIG_LOG) ? log_clock_event : NULL,
          IS_ENABLED(CONFIG_LOG) ? &clock_event_info : NULL);

static void profile_clock_power_event(struct log_event_buf *buf,
                  const struct event_header *eh)
{
    struct clock_power_event *event = cast_clock_power_event(eh);

    ARG_UNUSED(event);
    profiler_log_encode_u32(buf, event->p_msg->domain_id);
}

static int log_clock_power_event(const struct event_header *eh, char *buf,
             size_t buf_len)
{
    struct clock_power_event *event = cast_clock_power_event(eh);

    return snprintf(buf, buf_len, "domain=%d", event->p_msg->domain_id);
}

EVENT_INFO_DEFINE(clock_power_event,
          ENCODE(PROFILER_ARG_U8),
          ENCODE("domain"),
          profile_clock_power_event);

EVENT_TYPE_DEFINE(clock_power_event,
	  IS_ENABLED(CONFIG_LOG) ? true : false,
	  IS_ENABLED(CONFIG_LOG) ? log_clock_power_event : NULL,
	  IS_ENABLED(CONFIG_LOG) ? &clock_power_event_info : NULL);


static int log_clock_done_event(const struct event_header *eh, char *buf,
             size_t buf_len)
{
    struct clock_done_event *event = cast_clock_done_event(eh);

    return snprintf(buf, buf_len, "domain=%d", event->p_msg->domain_id);
}

EVENT_INFO_DEFINE(clock_done_event,
          ENCODE(PROFILER_ARG_U8),
          ENCODE("domain"),
          NULL);

EVENT_TYPE_DEFINE(clock_done_event,
	  IS_ENABLED(CONFIG_LOG) ? true : false,
	  IS_ENABLED(CONFIG_LOG) ? log_clock_done_event : NULL,
	  IS_ENABLED(CONFIG_LOG) ? &clock_done_event_info : NULL);



static int log_clock_source_event(const struct event_header *eh, char *buf,
             size_t buf_len)
{
    struct clock_source_event *event = cast_clock_source_event(eh);
    nrfs_gpms_clksrc_t *p_req = (nrfs_gpms_clksrc_t *) event->p_msg->p_buffer;

    return snprintf(buf, buf_len, "d=%d r=%d",
		    event->p_msg->domain_id,
		    p_req->data.request);
}

EVENT_INFO_DEFINE(clock_source_event,
          ENCODE(PROFILER_ARG_U8),
          ENCODE("domain"),
          NULL);

EVENT_TYPE_DEFINE(clock_source_event,
	  IS_ENABLED(CONFIG_LOG) ? true : false,
	  IS_ENABLED(CONFIG_LOG) ? log_clock_source_event : NULL,
	  IS_ENABLED(CONFIG_LOG) ? &clock_source_event_info : NULL);
