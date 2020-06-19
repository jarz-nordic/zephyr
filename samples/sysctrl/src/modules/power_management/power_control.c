/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(MAIN);

#include "status_event.h"
#include "prism_event.h"
#include "sleep_event.h"
#include "performance_event.h"

#include "pm/pm.h"
#include "pm/pcm/global_off.h"
#include "pm/pcm/performance.h"
#include "pm/pcm/dispatcher.h"
#include "pm/pcm/scheduler.h"

#include <nrfs_pm.h>

static void power_control_init(void)
{
	pm_pcm_dispatcher_init();
	pm_pcm_scheduler_init();
}

static void power_handle(struct pm_request_power *pwr)
{
	struct pm_return ret = pm_pcm_dispatcher_event_request(PM_PCM_DISPATCHER_EVENT_POWER, pwr);

	if (ret.response != PM_RESPONSE_NONE) {
		void *p_buffer = k_malloc(sizeof(ret));
		(void)memcpy(p_buffer, &ret, sizeof(ret));

		nrfs_phy_t *p_msg = k_malloc(sizeof(nrfs_phy_t));
		p_msg->p_buffer = p_buffer;
		p_msg->size = sizeof(ret);
		p_msg->ept_id = 0;
		p_msg->domain_id = pwr->domain;

		struct prism_event *prism_evt = new_prism_event();
		prism_evt->status = PRISM_MSG_STATUS_TX;
		prism_evt->p_msg = p_msg;
		EVENT_SUBMIT(prism_evt);
	}
}

static void sleep_handle(struct sleep_event *evt)
{
	nrfs_pm_sleep_t *p_req = (nrfs_pm_sleep_t *)evt->p_msg->p_buffer;

	LOG_DBG("Got off, domain = %d, time = %08x%08x",
		evt->p_msg->domain_id,
		(u32_t)((p_req->data.time & 0xFFFFFFFF00000000) >> 32),
		(u32_t)(p_req->data.time & 0x00000000FFFFFFFF));

	/* Translate nrfs structure to pm structure.
	   Won't be needed in the future  when the pm API will use nrfs. */

	struct pm_request_sleep sleep_req;
	sleep_req.domain = evt->p_msg->domain_id;
	sleep_req.state = (enum pm_sleep)p_req->data.state;
	sleep_req.time = p_req->data.time;
	sleep_req.latency = p_req->data.latency;

	/* Transform pm_request_sleep into pm_request_power */
	struct pm_request_power req = pm_pcm_global_off_into_power(&sleep_req);

	power_handle(&req);

	struct prism_event *prism_evt = new_prism_event();
	prism_evt->p_msg = evt->p_msg;
	prism_evt->status = PRISM_MSG_STATUS_RX_RELEASED;
	EVENT_SUBMIT(prism_evt);
}

static void performance_handle(struct performance_event *evt)
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
		struct status_event *evt = cast_status_event(eh);
		if (evt->status == STATUS_INIT) {
			power_control_init();
		}
		return false;
	}

	if (is_sleep_event(eh)) {
		struct sleep_event *evt = cast_sleep_event(eh);
		sleep_handle(evt);
		return true;
	}

	if (is_performance_event(eh)) {
		struct performance_event *evt = cast_performance_event(eh);
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
