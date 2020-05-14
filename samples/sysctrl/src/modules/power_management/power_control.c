/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(MAIN);

#include "status_event.h"
#include "sleep_event.h"
#include "performance_event.h"
#include "send_event.h"

#include "pm/pm.h"
#include "pm/pcm/global_off.h"
#include "pm/pcm/performance.h"
#include "pm/pcm/dispatcher.h"
#include "pm/pcm/scheduler.h"

static void power_control_init(void)
{
	pm_pcm_dispatcher_init();
	pm_pcm_scheduler_init();
}

static void power_handle(struct pm_request_power *pwr)
{
	struct pm_return ret = pm_pcm_dispatcher_event_request(PM_PCM_DISPATCHER_EVENT_POWER, pwr);

	if (ret.response != PM_RESPONSE_NONE)
	{
		/* Add send event with response. */
		struct send_event * send = new_send_event();
		send->size = sizeof(ret);
		send->endpoint = pwr->domain;
		__ASSERT_NO_MSG(send->size <= 200);
		memcpy(send->payload, &ret, send->size);
		EVENT_SUBMIT(send);
	}
}

static void sleep_handle(struct sleep_event * evt)
{
	LOG_DBG("Got off, domain = %d, time = %08x%08x",
		evt->request.domain,
		(u32_t)((evt->request.time & 0xFFFFFFFF00000000) >> 32),
		(u32_t)(evt->request.time & 0x00000000FFFFFFFF));

	/* Transform pm_request_sleep into pm_request_power */
	struct pm_request_power req = pm_pcm_global_off_into_power(&evt->request);

	power_handle(&req);
}

static void performance_handle(struct performance_event * evt)
{
	LOG_DBG("Got performance, domain = %d, time = %08x%08x",
		evt->request.domain,
		(u32_t)((evt->request.time & 0xFFFFFFFF00000000) >> 32),
		(u32_t)(evt->request.time & 0x00000000FFFFFFFF));

	/* Transform pm_request_performance into pm_request_power */
	struct pm_request_power req = pm_pcm_performance_into_power(&evt->request);

	power_handle(&req);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_status_event(eh)) {
		struct status_event * evt = cast_status_event(eh);
		if (evt->status == STATUS_INIT)
		{
			power_control_init();
		}
		return false;
	}

	if (is_sleep_event(eh)) {
		struct sleep_event * evt = cast_sleep_event(eh);
		sleep_handle(evt);
		return true;
	}

	if (is_performance_event(eh)) {
		struct performance_event * evt = cast_performance_event(eh);
		performance_handle(evt);
		return true;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(POWER_CONTROL, event_handler);
EVENT_SUBSCRIBE(POWER_CONTROL, status_event);
EVENT_SUBSCRIBE(POWER_CONTROL, sleep_event);
EVENT_SUBSCRIBE(POWER_CONTROL, performance_event);
