/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include "gpms_event.h"

struct gpms_event *new_gpms_event(void)
{
	static struct gpms_event evt;

	return &evt;
}
