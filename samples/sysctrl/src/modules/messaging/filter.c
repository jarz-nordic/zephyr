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
#include "led_event.h"
#include "sleep_event.h"

#include <nrfs_hdr.h>

static void filter_init(void)
{
	LOG_INF("Filter initialized");
}

static void *pm_msg_filter(nrfs_phy_t *p_msg)
{
	nrfs_hdr_t *p_hdr = NRFS_HDR_GET(p_msg);
	void *p_evt = NULL;

	switch (p_hdr->req) {
	case NRFS_PM_REQ_SLEEP:
		p_evt = new_sleep_event();
		struct sleep_event *sleep_evt = cast_sleep_event(p_evt);
		sleep_evt->p_msg = p_msg;
		break;

	case NRFS_PM_REQ_PERF:
		break;

	default:
		break;
	}

	return p_evt;
}

static void *led_msg_filter(nrfs_phy_t *p_msg)
{
	struct led_event *led_evt = new_led_event();

	led_evt->p_msg = p_msg;
	return led_evt;
}

static void * (*const filters[])(nrfs_phy_t * p_msg) =
{
	[NRFS_SERVICE_ID_LED] = led_msg_filter,
	[NRFS_SERVICE_ID_PM] = pm_msg_filter,
};

static void msg_received(struct prism_event *evt)
{
	LOG_INF("Filter got msg from domain %d, endpoint %u, size: %u",
		evt->p_msg->domain_id, evt->p_msg->ept_id, evt->p_msg->size);

	nrfs_hdr_t *p_hdr = NRFS_HDR_GET(evt->p_msg);
	uint8_t srv_id = NRFS_SERVICE_ID_GET(p_hdr->req);

	void *p_evt = NULL;
	if (srv_id < ARRAY_SIZE(filters)) {
		p_evt = filters[srv_id](evt->p_msg);
	}

	if (!p_evt) {
		p_evt = new_prism_event();
		struct prism_event *prism_evt = cast_prism_event(p_evt);
		prism_evt->p_msg = evt->p_msg;
		prism_evt->status = PRISM_MSG_STATUS_RX_RELEASED;
		LOG_ERR("Request type could not be determined: %u", p_hdr->req);
	}

	_event_submit(p_evt);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_status_event(eh)) {
		struct status_event *evt = cast_status_event(eh);
		if (evt->status == STATUS_INIT) {
			filter_init();
		}
		return false;
	}

	if (is_prism_event(eh)) {
		struct prism_event *evt = cast_prism_event(eh);
		if (evt->status == PRISM_MSG_STATUS_RX) {
			msg_received(evt);
			return true;
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(FILTER, event_handler);
EVENT_SUBSCRIBE(FILTER, status_event);
EVENT_SUBSCRIBE(FILTER, prism_event);
