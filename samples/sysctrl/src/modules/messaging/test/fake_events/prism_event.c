/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "prism_event.h"

void prism_event_release(nrfs_phy_t *msg)
{
}

bool is_prism_event(const struct event_header *eh)
{
	return true;
}

struct prism_event *cast_prism_event(const struct event_header *eh)
{
	struct prism_event *event;

	event = CONTAINER_OF(eh, struct prism_event, header);

	return event;
}
