/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <zephyr.h>
#include <logging/log.h>

#include <device.h>
#include <soc.h>

#include <stdio.h>
#include <string.h>
#include <init.h>

#include <prism_dispatcher.h>

#define PAYLOAD "fromNRF53app"

#define PRISM_DOMAIN_MAX_COUNT (8)

#define IRQ_SEM_MAX_COUNT	  \
	(PRISM_DOMAIN_MAX_COUNT * \
	 CONFIG_PRISM_DISPATCHER_BACKEND_OPENAMP_VRING_SIZE)

#define SEND_REPEAT_COUNT (3)

#define THREAD_STACK_SIZE (1024)

LOG_MODULE_REGISTER(nrf53app, CONFIG_LOG_DEFAULT_LEVEL);

K_SEM_DEFINE(m_sem_irq, 0, IRQ_SEM_MAX_COUNT);

struct k_thread m_rx_thread_cb;
struct k_thread m_tx_thread_cb;

K_THREAD_STACK_DEFINE(m_rx_thread_stack, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(m_tx_thread_stack, THREAD_STACK_SIZE);

/**
 * @brief Function for handling IPC IRQs.
 *
 * This function is registered inside Prism Dispatcher and invoked every time
 * IPC transport backend receives IPC IRQ signal.
 */
void prism_irq_handler(void)
{
	LOG_INF("IRQ handler invoked.");
	k_sem_give(&m_sem_irq);
}

/**
 * @brief Function for handling endpoint callbacks.
 *
 * This function is invoked every time IPC transport backend receives a valid message.
 * It is common for all registered endpoints.
 * Message contents are obtained using @ref prism_dispatcher_recv().
 */
static void eptx_handler(void)
{
	prism_dispatcher_msg_t msg;

	prism_dispatcher_err_t status = prism_dispatcher_recv(&msg);

	if (status != PRISM_DISPATCHER_OK) {
		LOG_ERR("Receive failed: %d", status);
	}

	LOG_INF("[Domain: %d | Ept: %u] handler invoked.", msg.domain_id, msg.ept_id);

	status = prism_dispatcher_free(&msg);
	if (status != PRISM_DISPATCHER_OK) {
		LOG_ERR("Free failed: %d", status);
	}
}

/** @brief Array holding endpoints configurations. */
static const prism_dispatcher_ept_t m_epts[] =
{
	{ PRISM_DOMAIN_APP,    10, eptx_handler },
	{ PRISM_DOMAIN_APP,    20, eptx_handler },
	{ PRISM_DOMAIN_NET,    30, eptx_handler },
	{ PRISM_DOMAIN_NET,    40, eptx_handler },
	{ PRISM_DOMAIN_SECURE, 50, eptx_handler },
	{ PRISM_DOMAIN_SECURE, 60, eptx_handler },
	{ PRISM_DOMAIN_MODEM,  70, eptx_handler },
	{ PRISM_DOMAIN_MODEM,  80, eptx_handler },
};

/** @brief Function for handling incoming IPC messages. */
void rx_thread(void *arg1, void *arg2, void *arg3)
{
	while (1) {
		k_sem_take(&m_sem_irq, K_FOREVER);
		prism_dispatcher_err_t status = prism_dispatcher_process(NULL);
		if (status != PRISM_DISPATCHER_OK && status != PRISM_DISPATCHER_ERR_NO_MSG) {
			LOG_ERR("Process failed: %d", status);
		}
	}
}

/** @brief Function simulating asynchronous message sending. */
void tx_thread(void *arg1, void *arg2, void *arg3)
{
	prism_dispatcher_msg_t msg =
	{
		.payload = PAYLOAD,
		.size = sizeof(PAYLOAD),
	};

	prism_dispatcher_err_t status;

	for (int i = 0; i < SEND_REPEAT_COUNT; i++) {
		for (size_t ept_idx = 0; ept_idx < ARRAY_SIZE(m_epts); ept_idx++) {
			prism_dispatcher_ept_t const *p_ept = &m_epts[ept_idx];
			msg.domain_id = p_ept->domain_id;
			msg.ept_id = p_ept->ept_id;

			LOG_INF("Sending message to domain: %d, ept: %u", msg.domain_id, msg.ept_id);

			// Wait for endpoint to become connected.
			while ((status = prism_dispatcher_send(&msg)) == PRISM_DISPATCHER_ERR_NO_CONN) {
				k_sleep(K_USEC(50));
			}

			if (status != PRISM_DISPATCHER_OK) {
				LOG_ERR("Sending failed: %d", status);
			}
			k_sleep(K_USEC(50));
		}
	}
}

int main(void)
{
	LOG_INF("System controller.");

	prism_dispatcher_err_t status;
	status = prism_dispatcher_init(prism_irq_handler, m_epts, ARRAY_SIZE(m_epts));
	if (status != PRISM_DISPATCHER_OK) {
		LOG_ERR("Dispatcher init failed: %d", status);
	}

	k_thread_create(&m_rx_thread_cb,
			m_rx_thread_stack,
			K_THREAD_STACK_SIZEOF(m_rx_thread_stack),
			rx_thread,
			NULL, NULL, NULL,
			0, 0, K_NO_WAIT);

	k_thread_create(&m_tx_thread_cb,
			m_tx_thread_stack,
			K_THREAD_STACK_SIZEOF(m_tx_thread_stack),
			tx_thread,
			NULL, NULL, NULL,
			0, 0, K_NO_WAIT);

	return 0;
}
