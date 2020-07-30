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
#include "gpms_event.h"

#include <internal/nrfs_hdr.h>

static void filter_init(void)
{
	LOG_INF("Filter initialized");
}

static void *led_msg_filter(nrfs_phy_t *p_msg)
{
	struct led_event *led_evt = new_led_event();

	led_evt->p_msg = p_msg;
	return led_evt;
}

static void *gpms_msg_filter(nrfs_phy_t *p_msg)
{
    struct gpms_event *gpms_evt = new_gpms_event();

    gpms_evt->p_msg = p_msg;
    return gpms_evt;
}

static void * (*const filters[])(nrfs_phy_t * p_msg) =
{
	[NRFS_SERVICE_ID_LED] = led_msg_filter,
	[NRFS_SERVICE_ID_GPMS] = gpms_msg_filter,
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
