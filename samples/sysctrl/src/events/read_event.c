/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "read_event.h"


static void profile_read_event(struct log_event_buf *buf,
			       const struct event_header *eh)
{
}

EVENT_INFO_DEFINE(read_event,
		  ENCODE(),
		  ENCODE(),
		  profile_read_event);

EVENT_TYPE_DEFINE(read_event,
		  true,
		  NULL,
		  &read_event_info);
