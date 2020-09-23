/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(MAIN);

#include "status_event.h"
#include "mts_event.h"
#include "prism_event.h"
#include "ncm.h"

#include <internal/services/nrfs_mts.h>

static void mts_init(void)
{
	LOG_INF("Memory Transfer Service initialized");
}

static void mts_handle(struct mts_event *evt)
{
	nrfs_generic_t *p_req = (nrfs_generic_t *)evt->p_msg->p_buffer;

	switch (p_req->hdr.req) {
	case NRFS_MTS_REQ_COPY:
	{
		nrfs_mts_copy_t *p_copy = (nrfs_mts_copy_t *)p_req;
		(void)memcpy(p_copy->data.p_sink, p_copy->data.p_source, p_copy->data.size);

		if (p_copy->data.do_notify) {
			struct ncm_ctx ctx;
			ncm_fill(&ctx, evt->p_msg);

			nrfs_mts_copy_rsp_data_t *p_rsp = ncm_alloc(sizeof(nrfs_mts_copy_rsp_data_t));
			p_rsp->p_source = p_copy->data.p_source;
			p_rsp->p_sink = p_copy->data.p_sink;
			p_rsp->size = p_copy->data.size;

			ncm_notify(&ctx, p_rsp, sizeof(*p_rsp));
		}
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
			mts_init();
		}
		return false;
	}

	if (is_mts_event(eh)) {
		struct mts_event *evt = cast_mts_event(eh);
		mts_handle(evt);
		return true;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MTS, event_handler);
EVENT_SUBSCRIBE(MTS, status_event);
EVENT_SUBSCRIBE(MTS, mts_event);
