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
#include "clock_event.h"
#include "sleep_event.h"
#include "performance_event.h"
#include "radio_event.h"
#include "gpms_event.h"
#include "gpms_notify_event.h"

#include "pm/pm.h"
#include "pm/pcm/global_off.h"
#include "pm/pcm/performance.h"
#include "pm/pcm/dispatcher.h"
#include "pm/pcm/scheduler.h"
#include "ncm.h"

#include <nrfs_gpms.h>

static struct {
	uint32_t dummy_service_specific_data;
	struct ncm_ctx ctx;
} power_control_ctx[1]; /* TODO: should be probably defined via Kconfig */

static inline bool is_gpms_fault(int32_t gpms_status)
{
	return (bool) gpms_status;
}

static void power_control_init(void)
{
	pm_pcm_dispatcher_init();
	pm_pcm_scheduler_init();
}

static void power_handle(struct pm_request_power *pwr)
{
	struct pm_return ret = pm_pcm_dispatcher_event_request(PM_PCM_DISPATCHER_EVENT_POWER, pwr);

	if (ret.response != PM_RESPONSE_NONE) {
		void *p_buffer = ncm_alloc(sizeof(ret));
		(void)memcpy(p_buffer, &ret, sizeof(ret));
		ncm_notify(&power_control_ctx[0].ctx, p_buffer, sizeof(ret));
	}
}

/* @brief Function for notifying GPMS.
 * @param p_resp    Pointer to the service response structure.
 * @param resp_size Size of the service response structure.
 * @param p_ctx     Pointer to the context.
 * @param result    Result of the operation.
 * @retval 0       Notification sent with success.
 *         -ENOMEM Failed to allocate memory for the message.
 */
static int pm_notify(void *p_resp, size_t resp_size, uint32_t ctx, int32_t result)
{
	struct gpms_notify_event *notify_evt = new_gpms_notify_event();

	/* Allocate memory for the response. */
	void *p_resp_buffer = ncm_alloc(resp_size);

	if (p_resp_buffer == NULL) {
		/* Failed to allocate NCM memory. */
		return -ENOMEM;
	}

	/* Copy provided response structure to the response buffer. */
	memcpy(p_resp_buffer, p_resp, resp_size);

	/* Assign pointer to the response structure to the event. */
	notify_evt->p_msg = p_resp_buffer;
	notify_evt->msg_size = resp_size;
	notify_evt->ctx = ctx;
	notify_evt->status = result;

	/* Issue the event.*/
	EVENT_SUBMIT(notify_evt);

	return 0;
}

static void clock_handle(const struct clock_event *evt)
{
	nrfs_gpms_clock_t *p_req = (nrfs_gpms_clock_t *) evt->p_msg->p_buffer;
	int32_t result = 0;

	/* Check if GPMS failed or not. */
	if (is_gpms_fault(evt->status)) {
		/* GPMS encountered severe error. Rewrite error code
		 * to the clock structure and return response.
		 * Or assign other error code - TBD. */
		result = evt->status;
	} else {
		/* TODO */
		result = 5;
	}

	/* Operation is done. We can handle response if needed. */
	nrfs_gpms_clock_rsp_data_t gpms_clock_res_data;
	uint32_t msg_ctx = (uint32_t)p_req->ctx.ctx;

	gpms_clock_res_data.response = result;

	if (pm_notify(&gpms_clock_res_data,
		      sizeof(gpms_clock_res_data),
		      msg_ctx,
		      result) != 0) {
		__ASSERT(false, "Failed to allocate memory for the clock response data.");
	}

	/* Now we can release previous message, since it is not needed now.*/
	prism_event_release(evt->p_msg);
}

static void sleep_handle(const struct sleep_event *evt)
{
	nrfs_gpms_sleep_t *p_req = (nrfs_gpms_sleep_t *)evt->p_msg->p_buffer;
	int32_t result;

	LOG_DBG("Got off, domain = %d, time = %08x%08x",
		evt->p_msg->domain_id,
		(uint32_t)((p_req->data.time & 0xFFFFFFFF00000000) >> 32),
		(uint32_t)(p_req->data.time & 0x00000000FFFFFFFF));

	/* Check if GPMS failed or not. */
	if (is_gpms_fault(evt->status)) {
		/* GPMS encountered severe error. Rewrite error code
		 * to the sleep structure and return response.
		 * Or assign other error code - TBD. */
		result = evt->status;
	} else {
		/* TODO */
		struct pm_request_sleep sleep_req;

		sleep_req.domain = evt->p_msg->domain_id;
		sleep_req.state = (enum pm_sleep)p_req->data.state;
		sleep_req.time = p_req->data.time;
		sleep_req.latency = p_req->data.latency;

		/* Dummy response. */
		result = 0;

	}

	/* Operation is done. We can handle response if needed. */
	nrfs_gpms_sleep_rsp_data_t gpms_sleep_res_data;
	uint32_t msg_ctx = (uint32_t)p_req->ctx.ctx;

	gpms_sleep_res_data.response = result;
	if (pm_notify(&gpms_sleep_res_data,
		      sizeof(gpms_sleep_res_data),
		      msg_ctx,
		      result) != 0) {
		__ASSERT(false, "Failed to allocate memory for the sleep response data.");
	}
	prism_event_release(evt->p_msg);
}

static void performance_handle(struct performance_event *evt)
{
	LOG_DBG("Got performance, domain = %d, time = %08x%08x",
		evt->request.domain,
		(uint32_t)((evt->request.time & 0xFFFFFFFF00000000) >> 32),
		(uint32_t)(evt->request.time & 0x00000000FFFFFFFF));

	/* Transform pm_request_performance into pm_request_power */
	struct pm_request_power req = pm_pcm_performance_into_power(&evt->request);

	power_handle(&req);
}

static void radio_handle(const struct radio_event *evt)
{
	nrfs_gpms_radio_t *p_req = (nrfs_gpms_radio_t *)evt->p_msg->p_buffer;
	int32_t result = 0;

	/* Check if GPMS failed or not. */
	if (is_gpms_fault(evt->status)) {
		/* GPMS encountered severe error. Rewrite error code
		 * to the radio structure and return response.
		 * Or assign other error code - TBD. */
		result = evt->status;
	} else {
		/* TODO */
		if (p_req->data.request != NRFS_RADIO_OP_ON) {
			result = 1;
		}
	}

	/* Operation is done. We can handle response if needed. */
	nrfs_gpms_radio_rsp_data_t gpms_radio_res_data;
	uint32_t msg_ctx = (uint32_t)p_req->ctx.ctx;

	gpms_radio_res_data.response = result;

	if (pm_notify(&gpms_radio_res_data,
		      sizeof(gpms_radio_res_data),
		      msg_ctx,
		      result) != 0) {
		__ASSERT(false, "Failed to allocate memory for the radio response data.");
	}

	/* Now we can release previous message, since it is not needed now.*/
	prism_event_release(evt->p_msg);
}


static bool event_handler(const struct event_header *eh)
{
	if (is_status_event(eh)) {
		const struct status_event *evt = cast_status_event(eh);

		if (evt->status == STATUS_INIT) {
			power_control_init();
		}
		return false;
	}

	if (is_clock_event(eh)) {
		const struct clock_event *evt = cast_clock_event(eh);

		clock_handle(evt);
		return true;
	}

	if (is_sleep_event(eh)) {
		const struct sleep_event *evt = cast_sleep_event(eh);

		sleep_handle(evt);
		return true;
	}

	if (is_performance_event(eh)) {
		struct performance_event *evt = cast_performance_event(eh);

		performance_handle(evt);
		return true;
	}

	if (is_radio_event(eh)) {
		const struct radio_event *evt = cast_radio_event(eh);

		radio_handle(evt);
		return true;
	}
	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(POWER_CONTROL, event_handler);
EVENT_SUBSCRIBE(POWER_CONTROL, status_event);
EVENT_SUBSCRIBE(POWER_CONTROL, clock_event);
EVENT_SUBSCRIBE(POWER_CONTROL, sleep_event);
EVENT_SUBSCRIBE(POWER_CONTROL, performance_event);
EVENT_SUBSCRIBE(POWER_CONTROL, radio_event);
