/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __CONFIG_H_
#define __CONFIG_H_

#define CONFIG_MOTOR_PWM_PERIOD_US	(9500u)
#define CONFIG_DEFAULT_SPEED_MAX 	380 // pulses per 100us
#define CONFIG_DEFAULT_SPEED_MIN	250 // pulses per 100us
/* 1 rotation = 64 pulses = ~9cm */
#define CONFIG_DEFAULT_DISTANCE_MIN	3200 // pulses = ~4,5m
#define CONFIG_DEFAULT_DISTANCE_BRAKE	2135 // pulses = ~3m
#define CONFIG_DEFAULT_START_POWER	(CONFIG_MOTOR_PWM_PERIOD_US * 0.4)
#define CONFIG_DEFAULT_PID_KP		20
#define CONFIG_DEFAULT_PID_KI		1


enum config_param_type {
	CONFIG_PARAM_SPEED_MAX,
	CONFIG_PARAM_SPEED_MIN,
	CONFIG_PARAM_DISTANCE_MIN,	// minimum lap length in pulses
	CONFIG_PARAM_DISTANCE_BRAKE,
	CONFIG_PARAM_START_POWER,
	CONFIG_PARAM_PID_KP,
	CONFIG_PARAM_PID_KI,
	CONFIG_PARAM_BT_AUTH_PSWD,
	CONFIG_CHECKSUM,
	CONFIG_PARAM_MAX,
};

int config_module_init(void);
int config_param_write(enum config_param_type type, const void *data);
const void *config_param_get(enum config_param_type type);
void config_print_params(void);
int config_make_default_settings(void);

#endif // __CONFIG_H_
