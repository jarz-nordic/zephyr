/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(MAIN);

#include "clock_event.h"
#include "prism_event.h"
#include "gpms_notify_event.h"
#include "ncm.h"

struct clock_domain_t {
	enum nrfs_gpms_clock_frequency cpu_clk;
	bool waiting;
};

static struct clock_domain_t cpu_clk_02;
static struct clock_domain_t cpu_clk_03;

static void change_domain_freq(enum nrfs_gpms_clock_frequency cpu_clk, uint8_t domain_id) {
	/* Dummy */
	switch (domain_id) {
	case 0:
		cpu_clk_02.cpu_clk = cpu_clk;
		break;
	case 1:
		cpu_clk_03.cpu_clk = cpu_clk;
		break;
	}
	LOG_INF("Changed %d domain to %d clock.", domain_id, cpu_clk);
}

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
	nrfs_gpms_clock_t *p_req = (nrfs_gpms_clock_t *)evt->p_msg->p_buffer;

	struct clock_domain_t * cpu_clk;
	enum nrfs_gpms_clock_frequency req_freq = p_req->data.frequency;
	switch (evt->p_msg->domain_id) {
	case 0:
		cpu_clk = &cpu_clk_02;
		break;
	case 1:
		cpu_clk = &cpu_clk_03;
		break;
	default:
		LOG_ERR("Unsupported clock domain");
		prism_event_release(evt->p_msg);
		return;
	}


	if (req_freq != cpu_clk->cpu_clk) {
		struct clock_power_event *clk_evt = new_clock_power_event();
		clk_evt->p_msg = evt->p_msg;

		/** If requested frequency is lower than current frequency,
		 *  we change frequency immediately then send request to change power.
		 *  If requested frequency is higher than current frequency,
		 *  we send request to change power then change frequency after power
		 *  is modified.
		 */
		if (req_freq < cpu_clk->cpu_clk) {
			change_domain_freq(req_freq, evt->p_msg->domain_id);
			EVENT_SUBMIT(clk_evt);
		} else if (req_freq > cpu_clk->cpu_clk) {
			EVENT_SUBMIT(clk_evt);
			cpu_clk->waiting = true;
		}
	} else {
		/* Requested frequency same as current frequency, nothing to do */

		nrfs_gpms_clock_rsp_data_t gpms_clock_res_data;
		uint32_t msg_ctx = (uint32_t)p_req->ctx.ctx;

		/* Dummy response */
		uint32_t result = 16;
		gpms_clock_res_data.response = result;

		if (pm_notify(&gpms_clock_res_data,
			      sizeof(gpms_clock_res_data),
			      msg_ctx,
			      result) != 0) {
			__ASSERT(false, "Failed to allocate memory for the clock response data.");
		}
		prism_event_release(evt->p_msg);

	}
}

static void clock_done_handle(const struct clock_done_event *evt)
{
	struct clock_domain_t * cpu_clk;
	nrfs_gpms_clock_t *p_req = (nrfs_gpms_clock_t *)evt->p_msg->p_buffer;
	switch (evt->p_msg->domain_id) {
	case 0:
		cpu_clk = &cpu_clk_02;
		break;
	case 1:
		cpu_clk = &cpu_clk_03;
		break;
	default:
		LOG_ERR("Unsupported clock domain");
		return;
	}
	if (cpu_clk->waiting) {
		change_domain_freq(p_req->data.frequency, evt->p_msg->domain_id);
		cpu_clk->waiting = false;
	}

	nrfs_gpms_clock_rsp_data_t gpms_clock_res_data;
	uint32_t msg_ctx = (uint32_t)p_req->ctx.ctx;

	uint32_t result = 16;
	gpms_clock_res_data.response = result;

	if (pm_notify(&gpms_clock_res_data,
		      sizeof(gpms_clock_res_data),
		      msg_ctx,
		      result) != 0) {
		__ASSERT(false, "Failed to allocate memory for the clock response data.");
	}
	prism_event_release(evt->p_msg);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_clock_event(eh)) {
		const struct clock_event *evt = cast_clock_event(eh);

		clock_handle(evt);
		return false;
	}
	if (is_clock_done_event(eh)) {
		const struct clock_done_event *evt = cast_clock_done_event(eh);

		clock_done_handle(evt);
		return true;
	}

}

EVENT_LISTENER(CLOCK_CONTROL, event_handler);
EVENT_SUBSCRIBE(CLOCK_CONTROL, clock_event);
EVENT_SUBSCRIBE(CLOCK_CONTROL, clock_done_event);
