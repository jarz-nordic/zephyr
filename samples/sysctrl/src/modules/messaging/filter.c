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
#include "ncm.h"

#include <internal/nrfs_hdr.h>

enum filter_result {
	FILTER_OK,
	FILTER_SERVICE_UNRECOGNIZED,
	FILTER_REQUEST_UNRECOGNIZED,
	FILTER_REQUEST_REJECTED,
};

static void filter_init(void)
{
	LOG_INF("Filter initialized");
}

static void msg_reject(nrfs_phy_t *p_msg)
{
	struct ncm_ctx ctx;

	ncm_fill(&ctx, p_msg);
	NRFS_HDR_FILTER_ERR_SET(&ctx.hdr);
	void *buffer = ncm_alloc(0);
	ncm_notify(&ctx, buffer, 0);
}

static enum filter_result led_msg_filter(nrfs_phy_t *p_msg, void **pp_evt)
{
	/* Simulate events filtering*/
	static uint8_t cnt = 0;

	if (cnt++ % 2) {
		struct led_event *led_evt = new_led_event();
		led_evt->p_msg = p_msg;
		*pp_evt = led_evt;
		return FILTER_OK;
	} else   {
		return FILTER_REQUEST_REJECTED;
	}
}

static enum filter_result gpms_msg_filter(nrfs_phy_t *p_msg, void **pp_evt)
{
	struct gpms_event *gpms_evt = new_gpms_event();

	gpms_evt->p_msg = p_msg;
	*pp_evt = gpms_evt;
	return FILTER_OK;
}

static enum filter_result(*const filters[])(nrfs_phy_t * p_msg, void **pp_evt) =
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
	enum filter_result result = FILTER_SERVICE_UNRECOGNIZED;
	if (srv_id < ARRAY_SIZE(filters)) {
		result = filters[srv_id](evt->p_msg, &p_evt);
	}

	switch (result) {
	case FILTER_OK:
		_event_submit(p_evt);
		break;
	case FILTER_SERVICE_UNRECOGNIZED:
		LOG_ERR("Service unrecognized: %u", srv_id);
		break;
	case FILTER_REQUEST_UNRECOGNIZED:
		LOG_ERR("Request unrecognized: %u", p_hdr->req);
		break;
	case FILTER_REQUEST_REJECTED:
		LOG_ERR("Request rejected: %u", p_hdr->req);
		msg_reject(evt->p_msg);
		break;
	default:
		break;
	}

	if (result != FILTER_OK) {
		prism_event_release(evt->p_msg);
	}
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
