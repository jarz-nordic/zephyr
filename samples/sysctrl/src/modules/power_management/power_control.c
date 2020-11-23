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
#include "radio_event.h"
#include "gpms_event.h"

#include "pm/pm.h"
#include "pm/pcm/global_off.h"
#include "pm/pcm/performance.h"
#include "pm/pcm/dispatcher.h"
#include "pm/pcm/scheduler.h"
#include "ncm.h"
#include "gpms.h"

#include <internal/services/nrfs_gpms.h>
#include "nrf_gpms.h"
#include "pmgt_dt.h"

static inline bool is_gpms_fault(int32_t gpms_status)
{
	return (bool) gpms_status;
}

void *              dynamic[NRF_GPMS_MAX_NODES];
nrf_gpms_dyn_cons_t consumers[NRF_GPMS_MAX_NODES];
nrf_gpms_dyn_reg_t  regulators[NRF_GPMS_MAX_NODES];
nrf_gpms_dyn_rail_t rails[NRF_GPMS_MAX_NODES];

nrf_gpms_cb_t gpms = {
	.stat = lookup,
	.dyn  = dynamic,
	.cons = consumers,
	.reg  = regulators,
	.rail = rails,
};

static void power_control_init(void)
{
	pm_pcm_dispatcher_init();
	pm_pcm_scheduler_init();
	nrf_gpms_initialize(&gpms);
}

static void clock_power_handle(const struct clock_power_event *evt)
{
	nrfs_gpms_clock_t *p_req = (nrfs_gpms_clock_t *)evt->p_msg->p_buffer;

	const nrf_gpms_node_t * consumer;
	uint8_t mode;

	if (p_req->data.frequency > NRFS_GPMS_CPU_CLOCK_FREQUENCY_128_MHZ) {
		mode = 3;
	} else if (p_req->data.frequency > NRFS_GPMS_CPU_CLOCK_FREQUENCY_64_MHZ) {
		mode = 2;
	} else {
		mode = 1;
	}

	switch (evt->p_msg->domain_id) {
	case 0:
		consumer = &app_core;
		break;
	default:
		LOG_ERR("Unsupported clock domain");
		break;
	}
	nrf_gpms_consumer_mode_set(&gpms, consumer, mode);


	struct clock_done_event *clk_done = new_clock_done_event();
	clk_done->p_msg = evt->p_msg;

	EVENT_SUBMIT(clk_done);

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

	if (gpms_response(&gpms_radio_res_data,
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

	if (is_clock_power_event(eh)) {
		const struct clock_power_event *evt = cast_clock_power_event(eh);

		clock_power_handle(evt);
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
EVENT_SUBSCRIBE(POWER_CONTROL, clock_power_event);
EVENT_SUBSCRIBE(POWER_CONTROL, radio_event);
