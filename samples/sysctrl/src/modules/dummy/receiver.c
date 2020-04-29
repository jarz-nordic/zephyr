/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(MAIN);

#include "status_event.h"
#include "read_event.h"
#include "led_event.h"

#define READING_THREAD_STACK_SIZE	800
#define READING_THREAD_PRIORITY		10
#define READING_LED_EVT_NUM		4

void read_timer_expiry_fn(struct k_timer *dummy);
void led_timer_expiry_fn(struct k_timer *dummy);

static struct k_sem m_led_sem[READING_LED_EVT_NUM];
static struct k_sem m_read_sem;

static K_THREAD_STACK_DEFINE(reading_thread_stack, READING_THREAD_STACK_SIZE);
static struct k_thread m_reading_thread;

static K_TIMER_DEFINE(m_read_timer, read_timer_expiry_fn, NULL);
static K_TIMER_DEFINE(m_led_timer, led_timer_expiry_fn, NULL);


/* Create read event periodically */
void read_timer_expiry_fn(struct k_timer *dummy)
{
	struct read_event * evt = new_read_event();
	EVENT_SUBMIT(evt);
}

/* Release random led semaphores in random times */
void led_timer_expiry_fn(struct k_timer *dummy)
{
	uint8_t evt_id = sys_rand32_get() % 4;
	k_sem_give(&m_led_sem[evt_id]);

	uint32_t delay = sys_rand32_get() % 1000;
	LOG_INF("Setting led timer delay to %d ms", delay);
	k_timer_start(&m_led_timer, K_MSEC(delay), K_NO_WAIT);
}

static void reading_thread_fn(void)
{
	while (true) {
		/* Acquire reading semaphore */
		if(k_sem_take(&m_read_sem, K_FOREVER) == 0)
		{
			/* Foreach led endpoint, try to acquire its semaphore */
			for(uint8_t i = 0; i < READING_LED_EVT_NUM; i++)
			{
				if(k_sem_take(&m_led_sem[i], K_NO_WAIT) == 0)
				{
					/* If acquired semaphore, submit corresponding event */
					struct led_event * led_evt = new_led_event();
					led_evt->domain = i;
					EVENT_SUBMIT(led_evt);
				}
			}
		}
	}
}

static void read_init(void)
{
	/* Initialize led semaphores */
	for(uint8_t i = 0; i < READING_LED_EVT_NUM; i++)
	{
		k_sem_init(&m_led_sem[i], 0, 1);
	}

	/* Initialize read semaphore */
	k_sem_init(&m_read_sem, 0, 1);

	/* Create reading thread */
	k_thread_create(&m_reading_thread,
			reading_thread_stack,
			READING_THREAD_STACK_SIZE,
			(k_thread_entry_t)reading_thread_fn,
			NULL, NULL, NULL,
			READING_THREAD_PRIORITY,
			0, K_NO_WAIT);

	/* Start periodic read timer */
	k_timer_start(&m_read_timer, K_NO_WAIT, K_MSEC(1000));

	/* Start non-periodic led timer */
	k_timer_start(&m_led_timer, K_NO_WAIT, K_NO_WAIT);
}

static bool event_handler(const struct event_header *eh)
{
	if (is_read_event(eh)) {
		/* Release read semaphore */
		k_sem_give(&m_read_sem);
		return true;
	}

	if (is_status_event(eh)) {
		struct status_event * evt = cast_status_event(eh);
		if (evt->status == STATUS_INIT)
		{
			read_init();
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(RECEIVER, event_handler);
EVENT_SUBSCRIBE(RECEIVER, status_event);
EVENT_SUBSCRIBE(RECEIVER, read_event);
