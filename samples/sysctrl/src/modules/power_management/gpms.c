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
#include "prism_event.h"
#include "radio_event.h"
#include "ncm.h"

#include <nrfs_gpms.h>

typedef struct {
	struct ncm_ctx ctx_local;       /* Local context. Used to handle responses. */
	uint32_t ctx_sender;            /* Sender context. When message comes to the
	                                   gpms its context must be copied since it is
	                                   overwritten by local context. When response
	                                   is obtained sender context must be restored
	                                   to send message to the caller. */
	bool notification_request;
} node_ctx_t;

/* Statically allocated structure used for notifying local domain
 * when there is no memory for dynamic allocation. */
static struct ncm_ctx resp_ctx;

static sys_slist_t slist; /* Contains all collected messages as linked list. */

/* @brief Function for saving notification context.
 * @note  This function saves context of sender and adds it to
 *        the linked list. This context is used later in response to the
 *        sender (if there was notification request or response
 *        is different than 0).
 * @param p_evt     Pointer to the gpms event structure.
 * @param p_context Pointer to the context of the message.
 *                  It is context assigned by a sender. This context is
 *                  saved and overwritten by address of the node in linked list.
 * @retval 0          In case of success.
 *         -ECANCELED Failed to allocate memory for the node.
 */
static int gpms_handle_notification_ctx(const struct gpms_event *p_evt,
					uint32_t *p_context, bool notification_request)
{
	node_ctx_t *p_node_ctx = k_malloc(sizeof(node_ctx_t));

	if (p_node_ctx != NULL) {
		/* Save sender context in allocated node.*/
		ncm_fill(&p_node_ctx->ctx_local, p_evt->p_msg);
		p_node_ctx->ctx_sender = *p_context;
		p_node_ctx->notification_request = notification_request;

		sys_snode_t *node = (sys_snode_t *) p_node_ctx;

		sys_slist_append(&slist, node);

		/* Update context of the message. Assign address of the node
		 * in slist. Since the notification is required this address will be used
		 * by returning message to restore context of the sender.*/
		*p_context = (uint32_t) p_node_ctx;

	} else {
		/* There is not enough memory to allocate NCM context.
		 * Use statically allocated one.*/
		ncm_fill(&resp_ctx, p_evt->p_msg);
		return -ECANCELED;
	}
	return 0;
}

/* @brief Notification handler.
 * @note  This function sends notification to the local domain.
 * @param evt Pointer to the event structure.
 * @retval None.
 */
static void gpms_req_notify_handler(const struct gpms_event *evt)
{
	nrfs_gpms_notify_t *p_req = (nrfs_gpms_notify_t *) evt->p_msg->p_buffer;

	/* Get msg ctx from node list */
	node_ctx_t *p_node_ctx = (node_ctx_t *) p_req->ctx.ctx;

	bool is_notificaion_needed = (bool) (p_node_ctx->notification_request ||
					     p_req->data.result != 0);

	if (is_notificaion_needed) {
		/* Restore original context. */
		p_req->ctx.ctx = p_node_ctx->ctx_sender;

		/* Context is restored. We can notify local domain. */
		ncm_notify(&p_node_ctx->ctx_local, &p_req->data,
			   sizeof(nrfs_gpms_notify_data_t));
	} else {
		/* Notification is not needed. It is mandatory to free the p_req.
		 * If the notification is used, p_req is freed by @ref ncm_notify.*/
		k_free(p_req);
	}

	sys_snode_t *node = (sys_snode_t *) p_node_ctx;

	sys_slist_find_and_remove(&slist, node);

	k_free(p_node_ctx);
	k_free(evt->p_msg);
}

/* @brief Error handler.
 * @note  This function is called when GPMS is unable to forward the message
 *        to the controller. This function notifies local domain that error occured.
 * @param evt   Pointer to the event structure.
 * @param error Error code.
 * @retval None.
 */
static void gpms_error_handler(const struct gpms_event *evt, int error)
{
	/* Check for ECANCELED. It means that there was no memory for
	 * NCM context. In that case we can only release the dispatcher message. */
	nrfs_gpms_notify_data_t *p_buffer = ncm_alloc(sizeof(nrfs_gpms_notify_data_t));

	/* Assign @ref NRFS_GPMS_ERR_NOMEM since it is the only one error. */
	p_buffer->result = NRFS_GPMS_ERR_NOMEM;

	if (error != -ECANCELED) {
		/* There was enough memory to allocate NCM context, but not enough
		 * to allocate message. Use statically allocated data structure
		 * for safety reason. */
		struct ncm_srv_data *p_req = (struct ncm_srv_data *) evt->p_msg->p_buffer;

		/* Get msg ctx from node list */
		node_ctx_t *p_node_ctx = (node_ctx_t *) p_req->app_ctx.ctx;

		/* Restore original context. */
		p_req->app_ctx.ctx = p_node_ctx->ctx_sender;

		ncm_notify(&p_node_ctx->ctx_local, p_buffer, sizeof(nrfs_gpms_notify_data_t));

		/* Error notification sent. Release the node from the list. */
		sys_snode_t *node = (sys_snode_t *) p_node_ctx;

		sys_slist_find_and_remove(&slist, node);
		k_free(p_node_ctx);
	} else {
		/* Use statically allocated context to notify local domain. */
		ncm_notify(&resp_ctx, p_buffer, sizeof(nrfs_gpms_notify_data_t));
	}

	prism_event_release(evt->p_msg);
}

/* @brief Sleep request handler.
 * @note  This function sends sleep event to the PM controller.
 * @param evt Pointer to the event structure.
 * @retval None.
 */
static int gpms_req_sleep_handler(const struct gpms_event *evt)
{
	nrfs_gpms_sleep_t *p_req = (nrfs_gpms_sleep_t *) evt->p_msg->p_buffer;

	int result = gpms_handle_notification_ctx(evt, &p_req->ctx.ctx, false);

	if (result != 0) {
		__ASSERT(false, "Failed to allocate memory for the sleep msg ctx.");
		return result;
	}

	/* Forward the message further. */
	struct sleep_event *sleep_evt = new_sleep_event();

	if (sleep_evt) {
		sleep_evt->p_msg = evt->p_msg;
		EVENT_SUBMIT(sleep_evt);
		return 0;
	}
	return -ENOMEM;
}

/* @brief Performance request handler.
 * @note  This function is TBD.
 * @param evt Pointer to the event structure.
 * @retval None.
 */
static int gpms_req_perf_handler(const struct gpms_event *evt)
{
	/* TODO */
	return 0;
}

/* @brief Radio request handler.
 * @note  This function sends radio event to the PM controller.
 * @param evt Pointer to the event structure.
 * @retval None.
 */
static int gpms_req_radio_handler(const struct gpms_event *evt)
{
	nrfs_gpms_radio_t *p_req = (nrfs_gpms_radio_t *) evt->p_msg->p_buffer;

	int result = gpms_handle_notification_ctx(evt, &p_req->ctx.ctx, p_req->data.notify_done);

	if (result != 0) {
		__ASSERT(false, "Failed to allocate memory for the radio msg ctx.");
		return result;
	}

	/* Forward the message further. */
	struct radio_event *radio_evt = new_radio_event();

	if (radio_evt) {
		radio_evt->p_msg = evt->p_msg;
		EVENT_SUBMIT(radio_evt);
		return 0;
	}
	return -ENOMEM;
}

/* @brief Clock frequency request handler.
 * @note  This function sends clock frequency event to the PM controller.
 * @param evt Pointer to the event structure.
 * @retval None.
 */
static int gpms_req_clk_freq_handler(const struct gpms_event *evt)
{
	nrfs_gpms_clock_t *p_req = (nrfs_gpms_clock_t *) evt->p_msg->p_buffer;

	int result = gpms_handle_notification_ctx(evt, &p_req->ctx.ctx, p_req->data.notify_done);

	if (result != 0) {
		__ASSERT(false, "Failed to allocate memory for the clock msg ctx.");
		return result;
	}

	/* Forward the message further. */
	struct clock_event *clock_evt = new_clock_event();

	if (clock_evt) {
		clock_evt->p_msg = evt->p_msg;
		EVENT_SUBMIT(clock_evt);
		return 0;
	}
	return -ENOMEM;
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
	int result = 0;

	switch (hdr.req) {
	case NRFS_GPMS_REQ_SLEEP:
		result = gpms_req_sleep_handler(evt);
		break;

	case NRFS_GPMS_REQ_PERF:
		result = gpms_req_perf_handler(evt);
		break;

	case NRFS_GPMS_REQ_RADIO:
		result = gpms_req_radio_handler(evt);
		break;

	case NRFS_GPMS_REQ_CLK_FREQ:
		result = gpms_req_clk_freq_handler(evt);
		break;

	case NRFS_GPMS_REQ_NOTIFY:
		gpms_req_notify_handler(evt);
		break;

	default:
		/* Unsupported event type. Ignore message. */
		break;
	}
	if (result != 0) {
		gpms_error_handler(evt, result);
	}
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

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(GPMS, event_handler);
EVENT_SUBSCRIBE(GPMS, status_event);
EVENT_SUBSCRIBE(GPMS, gpms_event);
