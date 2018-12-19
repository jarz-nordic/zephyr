/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>

#define SENSORY_STACK_SIZE	(1024U)
static K_THREAD_STACK_DEFINE(sensors_thread_stack, SENSORY_STACK_SIZE);
static const char *thread_name = "sensory";
struct k_thread sensors_thread;
static k_thread_stack_t *stack = sensors_thread_stack;


extern s16_t temp_out;

static void sensors_thread_function(void *arg1, void *arg2, void *arg3)
{
	while (1) {
		printk("BT_temp = %d\n", temp_out);
		k_sleep(1000);
	}
}


int system_app_init(void)
{
	printk("test\n");

	k_tid_t tid = k_thread_create(&sensors_thread,
				      stack,
				      SENSORY_STACK_SIZE,
				      sensors_thread_function,
				      NULL,
				      NULL,
				      NULL,
				      K_LOWEST_APPLICATION_THREAD_PRIO,
				      0,
				      100);

	k_thread_name_set(tid, thread_name);

	return 0;
}



