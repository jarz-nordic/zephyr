/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "ld_notify_event.h"

bool is_ld_notify_event(const struct event_header *eh)
{
	return true;
}
