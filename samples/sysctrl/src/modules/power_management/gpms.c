/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <logging/log.h>
#include <sys/slist.h>
LOG_MODULE_DECLARE(MAIN);

#include "status_event.h"
#include "clock_event.h"
#include "sleep_event.h"
#include "gpms_event.h"
#include "gpms_notify_event.h"
#include "prism_event.h"
#include "radio_event.h"
#include "ncm.h"

#include <nrfs_gpms.h>

typedef struct {
	sys_snode_t sysnode;            /* Linked list node. */
	struct ncm_ctx ctx_local;       /* Local context. Used to handle responses. */
	bool notification_request;      /* Request from local domain to notify it about the
					   result. Even if it is not set, but service returns
					   error, it is sent to the local domain. */
	bool is_statically_allocated;   /* If there is not enough memory for the msg,
					   gpms module uses statically allocated
					   structure to notify about failure. */
} node_ctx_t;

/* Statically allocated structure used for notifying local domain
 * when there is no memory for dynamic allocation. */
static node_ctx_t rescue_node_ctx;

static sys_slist_t slist; /* Contains all collected messages as linked list. */

/* @brief Function for saving notification context.
 * @note  This function saves context of sender and adds it to
 *        the linked list. This context is used later in response to the
 *        sender (if there was notification request or response
 *        is different than 0).
 * @param p_evt      Pointer to the gpms event structure.
 * @param p_context  Pointer to the context of the message.
 *                   It is context assigned by a sender. This context is
 *                   saved and overwritten by address of the node in linked list.
 * @param notify_req Notification request from local domain.
 * @retval 0          In case of success.
 *         -ECANCELED Failed to allocate memory for the node.
 */
static int gpms_notify_ctx_save(const struct gpms_event *p_evt,
				uint32_t *p_context, bool notify_req)
{
	int result = 0;
	node_ctx_t *p_node_ctx = k_malloc(sizeof(node_ctx_t));

	if (p_node_ctx == NULL) {
		/* There is not enough memory to allocate NCM context.
		 * Use statically allocated one.*/
		p_node_ctx = &rescue_node_ctx;
		p_node_ctx->is_statically_allocated = true;
		result = -ECANCELED;
	}

	ncm_fill(&p_node_ctx->ctx_local, p_evt->p_msg);
	p_node_ctx->notification_request = notify_req;

	/* Add node to the linked list. */
	sys_slist_append(&slist, (sys_snode_t *)p_node_ctx);

	/* Update context of the message. Assign address of the node
	 * in slist. If the notification is required this address will be used
	 * by returning message to restore context of the sender.*/
	*p_context = (uint32_t) p_node_ctx;

	return result;
}

/* @brief  Function for removing context from the linked list.
 * @param  p_node_ctx Pointer to the linked list context.
 * @retval None.
 */
static void gpms_notify_ctx_remove(node_ctx_t *p_node_ctx)
{
	sys_slist_find_and_remove(&slist, &p_node_ctx->sysnode);
	if (!p_node_ctx->is_statically_allocated) {
		k_free(p_node_ctx);
	}
}

/* @brief Notification handler.
 * @note  This function sends notification to the local domain.
 * @param evt Pointer to the event structure.
 * @retval None.
 */
static void gpms_notification_handler(const struct gpms_notify_event *evt)
{
	node_ctx_t *p_node_ctx = (node_ctx_t *) evt->ctx;

	bool is_notification_needed = (bool) (p_node_ctx->notification_request ||
					      (evt->status != 0));

	if (is_notification_needed) {
		/* Send notification to the local domain. */
		ncm_notify(&p_node_ctx->ctx_local, evt->p_msg, evt->msg_size);
	} else   {
		/* Notification is not needed. Free the NCM context. */
		struct ncm_srv_data *p_data = CONTAINER_OF(evt->p_msg, struct ncm_srv_data, app_payload);

		ncm_free(p_data);
	}

	/* Free resources. */
	gpms_notify_ctx_remove(p_node_ctx);
	prism_event_release(evt->p_msg);
}

/* @brief Sleep request handler.
 * @note  This function sends sleep event to the PM controller.
 * @param evt Pointer to the event structure.
 * @retval None.
 */
static void gpms_handler_req_sleep(const struct gpms_event *evt)
{
	nrfs_gpms_sleep_t *p_req = (nrfs_gpms_sleep_t *) evt->p_msg->p_buffer;

	int result = gpms_notify_ctx_save(evt, &p_req->ctx.ctx, false);

	/* Forward the message further. */
	struct sleep_event *sleep_evt = new_sleep_event();

	if (sleep_evt) {
		sleep_evt->p_msg = evt->p_msg;
		sleep_evt->status = result;
		EVENT_SUBMIT(sleep_evt);
	}
}

/* @brief Performance request handler.
 * @note  This function is TBD.
 * @param evt Pointer to the event structure.
 * @retval None.
 */
static void gpms_handler_req_perf(const struct gpms_event *evt)
{
	/* TODO */
}

/* @brief Radio request handler.
 * @note  This function sends radio event to the PM controller.
 * @param evt Pointer to the event structure.
 * @retval None.
 */
static void gpms_handler_req_radio(const struct gpms_event *evt)
{
	nrfs_gpms_radio_t *p_req = (nrfs_gpms_radio_t *) evt->p_msg->p_buffer;

	int result = gpms_notify_ctx_save(evt, &p_req->ctx.ctx, p_req->data.do_notify);

	/* Forward the message further. */
	struct radio_event *radio_evt = new_radio_event();

	if (radio_evt) {
		radio_evt->p_msg = evt->p_msg;
		radio_evt->status = result;
		EVENT_SUBMIT(radio_evt);
	}
}

/* @brief Clock frequency request handler.
 * @note  This function sends clock frequency event to the PM controller.
 * @param evt Pointer to the event structure.
 * @retval None.
 */
static void gpms_handler_req_clk_freq(const struct gpms_event *evt)
{
	nrfs_gpms_clock_t *p_req = (nrfs_gpms_clock_t *) evt->p_msg->p_buffer;

	int result = gpms_notify_ctx_save(evt, &p_req->ctx.ctx, p_req->data.do_notify);

	/* Forward the message further. */
	struct clock_event *clock_evt = new_clock_event();

	if (clock_evt) {
		clock_evt->p_msg = evt->p_msg;
		clock_evt->status = result;
		EVENT_SUBMIT(clock_evt);
	}
}

/* @brief Initialization handler
 * @note  This function handles initialization event.
 * @param  None.
 * @retval None.
 */
static void gpms_init(void)
{
	sys_slist_init(&slist);
}

/* @brief GPMS handler.
 * @note  This function runs handler based on event.
 * @param evt Pointer to the event structure.
 * @retval None.
 */
static void gpms_handle(const struct gpms_event *evt)
{
	const struct ncm_srv_data *p_data =
		(const struct ncm_srv_data *) evt->p_msg->p_buffer;
	nrfs_hdr_t hdr = p_data->hdr;

	switch (hdr.req) {
	case NRFS_GPMS_REQ_SLEEP:
		gpms_handler_req_sleep(evt);
		break;

	case NRFS_GPMS_REQ_PERF:
		gpms_handler_req_perf(evt);
		break;

	case NRFS_GPMS_REQ_RADIO:
		gpms_handler_req_radio(evt);
		break;

	case NRFS_GPMS_REQ_CLK_FREQ:
		gpms_handler_req_clk_freq(evt);
		break;

	default:
		/* Unsupported event type. Ignore message. */
		break;
	}
	prism_event_release(evt->p_msg);
}

/* @brief Generic handler.
 * @note  This function checks the event and call proper handler.
 * @param eh Pointer to the event handler.
 * @retval None.
 */
static bool event_handler(const struct event_header *eh)
{
	if (is_status_event(eh)) {
		const struct status_event *evt = cast_status_event(eh);

		if (evt->status == STATUS_INIT) {
			gpms_init();
		}
		return false;
	}

	if (is_gpms_event(eh)) {
		const struct gpms_event *evt = cast_gpms_event(eh);

		gpms_handle(evt);
		return true;
	}

	if (is_gpms_notify_event(eh)) {
		const struct gpms_notify_event *evt = cast_gpms_notify_event(eh);

		gpms_notification_handler(evt);
		return true;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(GPMS, event_handler);
EVENT_SUBSCRIBE(GPMS, status_event);
EVENT_SUBSCRIBE(GPMS, gpms_event);
EVENT_SUBSCRIBE(GPMS, gpms_notify_event);
