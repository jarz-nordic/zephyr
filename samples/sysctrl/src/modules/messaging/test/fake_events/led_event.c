/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>

#include "led_event.h"

static struct led_event led_event;

struct led_event *new_led_event(void)
{
	return &led_event;
}
