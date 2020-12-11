/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include "clock_event.h"

struct clock_event *new_clock_event(void)
{
	static struct clock_event evt;

	return &evt;
}
