/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <event_manager.h>
#include <status_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MAIN, 4);

void main(void)
{
	LOG_ERR("Start");
	if (event_manager_init()) {
		LOG_ERR("Event Manager not initialized");
	} else {
		struct status_event *event = new_status_event();
		event->status = STATUS_INIT;
		EVENT_SUBMIT(event);
	}
}
