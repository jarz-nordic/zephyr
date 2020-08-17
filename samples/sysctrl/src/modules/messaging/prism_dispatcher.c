/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(MAIN);

#include "prism_event.h"
#include "status_event.h"
#include "ld_notify_event.h"

#include <prism_dispatcher.h>

K_SEM_DEFINE(m_sem_irq, 0, 255);
K_SEM_DEFINE(m_sem_free_msg, 1, 1);

struct k_thread m_rx_thread_cb;
K_THREAD_STACK_DEFINE(m_rx_thread_stack, 512);

static nrfs_phy_t m_nrfs_msg;
static bool m_msg_taken;

static void ipc_irq_handler(void)
{
	LOG_INF("IPC IRQ handler invoked.");
	k_sem_give(&m_sem_irq);
}

static void sysctrl_evt_handler(void)
{
	prism_dispatcher_msg_t msg;

	prism_dispatcher_recv(&msg);

	m_nrfs_msg.p_buffer = msg.payload;
	m_nrfs_msg.size = msg.size;
	m_nrfs_msg.domain_id = msg.domain_id;
	m_nrfs_msg.ept_id = msg.ept_id;

	struct prism_event *prism_evt = new_prism_event();
	prism_evt->p_msg = &m_nrfs_msg;
	prism_evt->status = PRISM_MSG_STATUS_RX;
	EVENT_SUBMIT(prism_evt);

	m_msg_taken = true;
}

static const prism_dispatcher_ept_t m_epts[] =
{
	{ PRISM_DOMAIN_APP, 0, sysctrl_evt_handler },
};

static void reading_thread_fn(void)
{
	while (true) {
		k_sem_take(&m_sem_irq, K_FOREVER);
		k_sem_take(&m_sem_free_msg, K_FOREVER);

		m_msg_taken = false;
		prism_dispatcher_err_t status = prism_dispatcher_process(NULL);
		if (status != PRISM_DISPATCHER_OK && status != PRISM_DISPATCHER_ERR_NO_MSG) {
			LOG_ERR("Dispatcher process failed: %d", status);
		}

		if (!m_msg_taken) {
			// If endpoint callback was not invoked, return the message buffer semaphore.
			k_sem_give(&m_sem_free_msg);
		}
	}
}

static void read_init(void)
{
	prism_dispatcher_err_t status;

	status = prism_dispatcher_init(ipc_irq_handler, m_epts, ARRAY_SIZE(m_epts));
	if (status != PRISM_DISPATCHER_OK) {
		LOG_ERR("Dispatcher init failed: %d", status);
	}

	k_thread_create(&m_rx_thread_cb,
			m_rx_thread_stack,
			K_THREAD_STACK_SIZEOF(m_rx_thread_stack),
			(k_thread_entry_t)reading_thread_fn,
			NULL, NULL, NULL,
			0, 0, K_NO_WAIT);

	LOG_INF("Prism dispatcher initialized");
}

static bool event_handler(const struct event_header *eh)
{
	if (is_status_event(eh)) {
		struct status_event *evt = cast_status_event(eh);
		if (evt->status == STATUS_INIT) {
			read_init();
		}
		return false;
	}

	if (is_prism_event(eh)) {
		struct prism_event *evt = cast_prism_event(eh);
		if (evt->status == PRISM_MSG_STATUS_RX_RELEASED) {
			prism_dispatcher_msg_t msg;
			msg.payload = evt->p_msg->p_buffer;
			prism_dispatcher_free(&msg);
			k_sem_give(&m_sem_free_msg);
			return true;
		}
		return false;
	}

	if (is_ld_notify_event(eh)) {
		struct ld_notify_event *evt = cast_ld_notify_event(eh);
		prism_dispatcher_msg_t msg = {
			.payload = evt->msg.p_buffer,
			.size = evt->msg.size,
			.ept_id = evt->msg.ept_id,
			.domain_id = evt->msg.domain_id,
		};
		prism_dispatcher_err_t status = prism_dispatcher_send(&msg);
		if (status != PRISM_DISPATCHER_OK) {
			LOG_ERR("Dispatcher send failed: %d", status);
		}
		ncm_free(evt->msg.p_buffer);
		return true;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(PRISM_DISPATCHER, event_handler);
EVENT_SUBSCRIBE(PRISM_DISPATCHER, status_event);
EVENT_SUBSCRIBE(PRISM_DISPATCHER, prism_event);
EVENT_SUBSCRIBE(PRISM_DISPATCHER, ld_notify_event);
