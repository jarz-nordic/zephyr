/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "mts_event.h"

struct mts_event *new_mts_event(void)
{
	static struct mts_event evt;

	return &evt;
}
