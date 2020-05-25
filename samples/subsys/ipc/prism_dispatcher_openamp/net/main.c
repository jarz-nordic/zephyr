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

#define PAYLOAD "fromApp"

LOG_MODULE_REGISTER(nrf53net, CONFIG_LOG_DEFAULT_LEVEL);

K_SEM_DEFINE(m_sem_irq, 0, 255);

struct k_thread m_rx_thread_cb;
struct k_thread m_tx_thread_cb;

K_THREAD_STACK_DEFINE(m_rx_thread_stack, 1024);
K_THREAD_STACK_DEFINE(m_tx_thread_stack, 1024);

void prism_irq_handler(void)
{
	LOG_INF("IRQ handler invoked.");
	k_sem_give(&m_sem_irq);
}

static void eptx_handler(void)
{
	prism_dispatcher_msg_t msg;

	prism_dispatcher_recv(&msg);

	LOG_INF("[Domain: %d | Ept: %u] handler invoked.", msg.domain_id, msg.ept_id);
	LOG_HEXDUMP_INF(msg.payload, msg.size, "Payload:");

	prism_dispatcher_free(&msg);
}

static const prism_dispatcher_ept_t m_epts[] =
{
	{ PRISM_DOMAIN_SYSCTRL, 10, eptx_handler },
	{ PRISM_DOMAIN_SYSCTRL, 20, eptx_handler },
	{ PRISM_DOMAIN_SYSCTRL, 30, eptx_handler },
};

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

void tx_thread(void *arg1, void *arg2, void *arg3)
{
	prism_dispatcher_msg_t msg =
	{
		.payload = PAYLOAD,
		.size = sizeof(PAYLOAD),
	};

	prism_dispatcher_err_t status;

	for (int i = 0; i < 3; i++) {
		for (size_t ept_idx = 0; ept_idx < ARRAY_SIZE(m_epts); ept_idx++) {
			prism_dispatcher_ept_t const *p_ept = &m_epts[ept_idx];
			msg.domain_id = p_ept->domain_id;
			msg.ept_id = p_ept->ept_id;

			LOG_INF("Sending message to domain: %d, ept: %u", msg.domain_id, msg.ept_id);
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
	LOG_INF("Application domain.");

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
