/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <logging/log.h>
#include <stdlib.h>
LOG_MODULE_DECLARE(MAIN);

#include "status_event.h"
#include "temp_event.h"
#include "prism_event.h"
#include "ncm.h"

#include <internal/services/nrfs_temp.h>

#define TEMP_MIN_MEASURE_RATE_MS 10

// TODO: These symbols should come from some global configuration.
#define TEMP_SUBSCRIBER_COUNT 5
#define TEMP_REQUESTER_COUNT 5

struct temp_subscriber {
	struct ncm_ctx ctx;
	int32_t last_temp;
	uint16_t measure_rate_ms;
	uint8_t threshold_factor;
	bool active;
};

struct temp_requester {
	struct ncm_ctx ctx;
	bool active;
};

static struct temp_subscriber subscribers[TEMP_SUBSCRIBER_COUNT];
static struct temp_requester requesters[TEMP_REQUESTER_COUNT];
static struct k_delayed_work temp_measure_work;

// TODO: This function should be called by the TEMP peripheral driver on a HW event.
static void measurement_ready_handler(int32_t raw_temp)
{
	for (size_t i = 0; i < ARRAY_SIZE(requesters); i++) {
		if (requesters[i].active) {
			requesters[i].active = false;
			nrfs_temp_rsp_data_t *p_data = ncm_alloc(sizeof(nrfs_temp_rsp_data_t));
			p_data->raw_temp = raw_temp;
			ncm_notify(&requesters[i].ctx, p_data, sizeof(nrfs_temp_rsp_data_t));
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		if (subscribers[i].active &&
		    abs(subscribers[i].last_temp - raw_temp) > subscribers[i].threshold_factor) {
			subscribers[i].last_temp = raw_temp;
			nrfs_temp_rsp_data_t *p_data = ncm_alloc(sizeof(nrfs_temp_rsp_data_t));
			p_data->raw_temp = raw_temp;
			ncm_notify(&subscribers[i].ctx, p_data, sizeof(nrfs_temp_rsp_data_t));
		}
	}
}

static void temp_measure_work_handler(struct k_work *p_work)
{
	ARG_UNUSED(p_work);

	// TODO: Trigger temperature measurement. For now invoke callback directly.
	static uint8_t i;
	measurement_ready_handler(80 + (i++ % 16));

	bool schedule_measurement = false;
	uint16_t next_minimal_measurement_rate = UINT16_MAX;
	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		if (subscribers[i].active) {
			schedule_measurement = true;
			if (subscribers[i].measure_rate_ms < next_minimal_measurement_rate) {
				next_minimal_measurement_rate = subscribers[i].measure_rate_ms;
			}
		}
	}

	if (schedule_measurement) {
		k_delayed_work_submit(&temp_measure_work, K_MSEC(next_minimal_measurement_rate));
	}
}

static void temp_init(void)
{
	k_delayed_work_init(&temp_measure_work, temp_measure_work_handler);
	LOG_INF("Temperature Service initialized");
}

static void temp_handle(struct temp_event *evt)
{
	nrfs_generic_t *p_req = (nrfs_generic_t *)evt->p_msg->p_buffer;

	switch (p_req->hdr.req) {
	case NRFS_TEMP_REQ_MEASURE:
	{
		ncm_fill(&requesters[evt->p_msg->domain_id].ctx, evt->p_msg);
		requesters[evt->p_msg->domain_id].active = true;
		break;
	}
	case NRFS_TEMP_REQ_SUBSCRIBE:
	{
		nrfs_temp_subscribe_t *p_sub_req = (nrfs_temp_subscribe_t *)evt->p_msg->p_buffer;
		struct temp_subscriber *p_subscriber = &subscribers[evt->p_msg->domain_id];

		ncm_fill(&p_subscriber->ctx, evt->p_msg);
		p_subscriber->threshold_factor = p_sub_req->data.threshold_factor;
		if (p_sub_req->data.measure_rate_ms > TEMP_MIN_MEASURE_RATE_MS) {
			p_subscriber->measure_rate_ms = p_sub_req->data.measure_rate_ms;
		} else {
			p_subscriber->measure_rate_ms = TEMP_MIN_MEASURE_RATE_MS;
		}
		p_subscriber->active = true;

		if (!k_delayed_work_remaining_get(&temp_measure_work)) {
			k_delayed_work_submit(&temp_measure_work, K_MSEC(p_subscriber->measure_rate_ms));
		}

		break;
	}
	case NRFS_TEMP_REQ_UNSUBSCRIBE:
	{
		subscribers[evt->p_msg->domain_id].active = false;
		break;
	}
	default:
		break;
	}

	prism_event_release(evt->p_msg);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_status_event(eh)) {
		struct status_event *evt = cast_status_event(eh);
		if (evt->status == STATUS_INIT) {
			temp_init();
		}
		return false;
	}

	if (is_temp_event(eh)) {
		struct temp_event *evt = cast_temp_event(eh);
		temp_handle(evt);
		return true;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(TEMP, event_handler);
EVENT_SUBSCRIBE(TEMP, status_event);
EVENT_SUBSCRIBE(TEMP, temp_event);
