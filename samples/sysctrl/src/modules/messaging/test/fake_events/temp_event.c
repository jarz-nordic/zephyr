/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "temp_event.h"

struct temp_event *new_temp_event(void)
{
	static struct temp_event evt;

	return &evt;
}
