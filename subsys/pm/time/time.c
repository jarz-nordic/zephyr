/*
 * Copyright (c) 2020 Nordic Semuconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include "pm/pm.h"
#include "pm/time/time.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(PM_PCM_TIME, 4);

int pm_time_callback_add(uint64_t time, pm_time_callback_fn callback, void *context)
{
	/* TODO */
	return 0;
}

uint64_t pm_time_rtc_get(void)
{
	/* TODO */
	return 0;
}
