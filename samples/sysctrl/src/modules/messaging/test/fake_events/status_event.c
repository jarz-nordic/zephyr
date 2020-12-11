/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "status_event.h"
#include <stdio.h>

bool is_status_event(const struct event_header *eh)
{
	return true;
}

struct status_event *cast_status_event(const struct event_header *eh)
{
	struct status_event *event;

	event = CONTAINER_OF(eh, struct status_event, header);

	return event;
}
