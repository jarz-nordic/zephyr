/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr.h>
#include <stats/stats.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>

LOG_MODULE_REGISTER(state_machine);

#define THREAD_STACK_SIZE	2048
#define THREAD_PRIORITY		K_PRIO_PREEMPT(1)

enum sm_states {
	SM_INIT,
	SM_CHECK_LICENSE,
	SM_IDLE,
	SM_MOTOR_FIRST_RUN,
	SM_MOTOR_RUN,
	SM_ERROR,
	SM_BT_UPDATE,
	SM_MAX
};

static enum sm_states state;

static K_THREAD_STACK_DEFINE(thread_stack, THREAD_STACK_SIZE);
static struct k_thread thread;

static void state_machine_thread_fn(void)
{

	while(1) {
		switch (state) {
		case SM_INIT:
			break;
		case SM_CHECK_LICENSE:
			break;
		case SM_IDLE:
			break;
		case SM_MOTOR_FIRST_RUN:
			break;
		case SM_MOTOR_RUN:
			break;
		case SM_ERROR:
			break;
		case SM_BT_UPDATE:
			break;
		default:
			LOG_ERR("%s: unexpected SM state.", __FUNCTION__);
			state = SM_ERROR;
			break;
        }

		k_sleep(K_MSEC(200));
	}
}

int state_machine_init(void)
{
	k_thread_create(&thread, thread_stack,
			THREAD_STACK_SIZE,
			(k_thread_entry_t)state_machine_thread_fn,
			NULL, NULL, NULL,
			THREAD_PRIORITY, 0, K_MSEC(300));
	k_thread_name_set(&thread, "state_machine_thread");

	return 0;
}

