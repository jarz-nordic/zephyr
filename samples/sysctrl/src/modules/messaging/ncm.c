/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "ncm.h"
#include "ld_notify_event.h"

#include <zephyr.h>

void ncm_fill(struct ncm_ctx *p_ctx, nrfs_phy_t *p_msg)
{
	struct ncm_srv_data *p_data = (struct ncm_srv_data *)p_msg->p_buffer;

	p_ctx->hdr = p_data->hdr;
	p_ctx->app_ctx = p_data->app_ctx;
	p_ctx->domain_id = p_msg->domain_id;
	p_ctx->ept_id = p_msg->ept_id;
}

uint32_t ncm_req_id_get(struct ncm_ctx *p_ctx)
{
	return (uint32_t)p_ctx->hdr.req;
}

void *ncm_alloc(size_t bytes)
{
	struct ncm_srv_data *payload = k_malloc(bytes + sizeof(struct ncm_srv_data));

	__ASSERT_NO_MSG(payload);

	void *app_payload = &payload->app_payload;
	return app_payload;
}

void ncm_notify(struct ncm_ctx *p_ctx, void *app_payload, size_t size)
{
	struct ncm_srv_data *payload = CONTAINER_OF(app_payload, struct ncm_srv_data, app_payload);

	payload->hdr = p_ctx->hdr;
	payload->app_ctx = p_ctx->app_ctx;

	struct ld_notify_event *ld_notify_evt = new_ld_notify_event();
	ld_notify_evt->msg.p_buffer = payload;
	ld_notify_evt->msg.size = size + sizeof(struct ncm_srv_data);
	ld_notify_evt->msg.domain_id = p_ctx->domain_id;
	ld_notify_evt->msg.ept_id = p_ctx->ept_id;

	EVENT_SUBMIT(ld_notify_evt);
}

void ncm_free(void *payload)
{
	k_free(payload);
}
