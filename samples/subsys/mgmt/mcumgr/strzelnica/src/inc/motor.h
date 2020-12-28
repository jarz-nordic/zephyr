/*
 * Copyright (c) 2020 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __MOTOR_H_
#define __MOTOR_H_

enum motor_drv_direction {
	MOTOR_DRV_NEUTRAL,
	MOTOR_DRV_FORWARD,
	MOTOR_DRV_BACKWARD,
	MOTOR_DRV_BRAKE,
	MOTOR_DRV_MAX
};

int motor_init(void);
int motor_move(enum motor_drv_direction direction, uint32_t speed);

#endif // __MOTOR_H_
