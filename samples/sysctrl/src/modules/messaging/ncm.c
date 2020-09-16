/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include "ncm.h"
#include "ld_notify_event.h"
#include <internal/services/nrfs_generic.h>

#include <zephyr.h>

#define NCM_SLAB_BLOCK_SIZE  (16)               /* Single block size. */
#define NCM_SLAB_BLOCK_CNT   (4)                /* Block count. */
#define NCM_SLAB_BLOCK_ALIGN (4)                /* Block alignment. */

K_MEM_SLAB_DEFINE(ncm_slab,
		  NCM_SLAB_BLOCK_SIZE,
		  NCM_SLAB_BLOCK_CNT,
		  NCM_SLAB_BLOCK_ALIGN);

static void local_domain_notify(void * p_data, size_t size, uint8_t domain_id, uint8_t ept_id)
{
	struct ld_notify_event *ld_notify_evt = new_ld_notify_event();

	ld_notify_evt->msg.p_buffer = p_data;
	ld_notify_evt->msg.size = size + sizeof(nrfs_generic_t);
	ld_notify_evt->msg.domain_id = domain_id;
	ld_notify_evt->msg.ept_id = ept_id;

	EVENT_SUBMIT(ld_notify_evt);
}

void ncm_fill(struct ncm_ctx *p_ctx, nrfs_phy_t *p_msg)
{
	nrfs_generic_t *p_data = (nrfs_generic_t *)p_msg->p_buffer;

	p_ctx->hdr = p_data->hdr;
	p_ctx->app_ctx = p_data->ctx;
	p_ctx->domain_id = p_msg->domain_id;
	p_ctx->ept_id = p_msg->ept_id;
}

uint32_t ncm_req_id_get(struct ncm_ctx *p_ctx)
{
	return (uint32_t)p_ctx->hdr.req;
}

void *ncm_alloc(size_t bytes)
{
	ARG_UNUSED(bytes);
	nrfs_generic_t *p_data = NULL;

	__ASSERT(bytes + sizeof(nrfs_generic_t) <= NCM_SLAB_BLOCK_SIZE,
		 "Requested memory size greater than available.");
	k_mem_slab_alloc(&ncm_slab, (void **)&p_data, K_NO_WAIT);

	__ASSERT_NO_MSG(p_data);

	void *app_payload = &p_data->payload;
	return app_payload;
}

void ncm_abandon(void *app_payload)
{
	void *p_data = CONTAINER_OF(app_payload, nrfs_generic_t, payload);
	k_mem_slab_free(&ncm_slab, &p_data);
}

void ncm_notify(struct ncm_ctx *p_ctx, void *app_payload, size_t size)
{
	nrfs_generic_t *p_data = CONTAINER_OF(app_payload, nrfs_generic_t, payload);

	p_data->hdr = p_ctx->hdr;
	p_data->ctx = p_ctx->app_ctx;

	local_domain_notify(p_data, size, p_ctx->domain_id, p_ctx->ept_id);
}

void ncm_unsolicited_notify(struct ncm_unsolicited_ctx *p_ctx, void *app_payload, size_t size)
{
	nrfs_generic_t *p_data = CONTAINER_OF(app_payload, nrfs_generic_t, payload);

	p_data->hdr.req = p_ctx->notif_id;
	NRFS_HDR_UNSOLICITED_SET(&p_data->hdr);
	p_data->ctx.ctx = (uintptr_t)NULL;

	local_domain_notify(p_data, size, p_ctx->domain_id, p_ctx->ept_id);
}

void ncm_free(void *p_data)
{
	k_mem_slab_free(&ncm_slab, &p_data);
}
