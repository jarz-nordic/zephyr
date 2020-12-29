/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr.h>
#include <stats/stats.h>
#include "led.h"
#include "buttons.h"
#include "encoder.h"
#include "motor.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>

LOG_MODULE_REGISTER(state_machine);

#define THREAD_STACK_SIZE	2048
#define THREAD_PRIORITY		K_PRIO_PREEMPT(1)

#define SM_SPEED_SAMPLE_MS		(K_MSEC(100u))
#define SM_SPEED_FIRST_SAMPLE_MS	(K_MSEC(300u))

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

struct track {
	bool direction;
	uint32_t track_length;
	uint32_t distance;	// current distance
};

struct motor {
	uint32_t requested_speed;
	uint32_t duty_cycle;
	bool active;
};

struct sm_cb {
	struct k_delayed_work work;

	enum sm_states state;
	struct track track;
	struct motor motor;
};

static struct sm_cb sm_cb;
static K_THREAD_STACK_DEFINE(thread_stack, THREAD_STACK_SIZE);
static struct k_thread thread;

static inline bool is_motor_active(void)
{
	return sm_cb.motor.active;
}

static inline void motor_active_set(bool enable)
{
	sm_cb.motor.active = enable;
}

void buttons_cb(enum button_name name, enum button_event evt)
{
	if (name == BUTTON_NAME_CALL) {
		if (evt == BUTTON_EVT_PRESSED_SHORT) {
			led_blink_fast(LED_GREEN, 3);
		} else {
			led_blink_slow(LED_GREEN, 3);
		}
		button_enable(BUTTON_NAME_CALL, true);
	} else if (name == BUTTON_NAME_SPEED) {
		if (evt == BUTTON_EVT_PRESSED_SHORT) {
			led_blink_fast(LED_RED, 3);
		} else {
			led_blink_slow(LED_RED, 3);
		}
		button_enable(BUTTON_NAME_SPEED, true);
	}
}

static inline void state_set(enum sm_states state)
{
	if (state < SM_MAX) {
		sm_cb.state = state;
	}
}

static void sample_motor(struct k_work *work)
{
	int32_t val;
	int ret;

	ret = encoder_get(&val);

	if ((val <= 2) && (val >= -2)) {
		motor_move(MOTOR_DRV_NEUTRAL, 0);
		return;
	}

	k_delayed_work_submit(&sm_cb.work, SM_SPEED_SAMPLE_MS);
}

static void state_machine_thread_fn(void)
{

	while(1) {
		switch (sm_cb.state) {
		case SM_INIT:
			state_set(SM_CHECK_LICENSE);
			break;
		case SM_CHECK_LICENSE:
			state_set(SM_MOTOR_FIRST_RUN);
			motor_move(MOTOR_DRV_FORWARD, 3000); //30%
			k_delayed_work_submit(&sm_cb.work,
					      SM_SPEED_FIRST_SAMPLE_MS);
			break;
		case SM_MOTOR_FIRST_RUN:
			if (is_motor_active()) {
				break;
			}
			break;
		case SM_IDLE:
			break;
		case SM_MOTOR_RUN:
			break;
		case SM_ERROR:
			break;
		case SM_BT_UPDATE:
			break;
		default:
			LOG_ERR("%s: unexpected SM state.", __FUNCTION__);
			break;
		}

		k_sleep(K_MSEC(500));
	}
}

int state_machine_init(void)
{
	int ret;

	ret = buttons_init(buttons_cb);
	if (ret) {
		LOG_ERR("buttons module not initialized. err:%d", ret);
		return ret;
	}

	k_delayed_work_init(&sm_cb.work, sample_motor);

	k_thread_create(&thread, thread_stack,
			THREAD_STACK_SIZE,
			(k_thread_entry_t)state_machine_thread_fn,
			NULL, NULL, NULL,
			THREAD_PRIORITY, 0, K_MSEC(300));
	k_thread_name_set(&thread, "state_machine_thread");

	return 0;
}

