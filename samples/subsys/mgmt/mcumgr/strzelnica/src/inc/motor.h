/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __MOTOR_H_
#define __MOTOR_H_

#include "config.h"

#define MOTOR_PWM_PERIOD_US		(CONFIG_MOTOR_PWM_PERIOD_US)
#define MOTOR_PWM_MAX_PERIOD_US		(1 * MOTOR_PWM_PERIOD_US)
#define MOTOR_PWM_MIN_PERIOD_US		(0.125*MOTOR_PWM_PERIOD_US) // 12,55555%

/* Enum with available motor actions */
enum motor_drv_direction {
	MOTOR_DRV_NEUTRAL,
	MOTOR_DRV_FORWARD,
	MOTOR_DRV_BACKWARD,
	MOTOR_DRV_BRAKE,
/* -------- Internal use only ----*/
	MOTOR_DRV_MAX
};

int motor_init(void);
int motor_move(enum motor_drv_direction direction, uint32_t speed);

#endif // __MOTOR_H_
