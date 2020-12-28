/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __CONFIG_H_
#define __CONFIG_H_

enum config_param_type {
	CONFIG_PARAM_SPEED_MAX,
	CONFIG_PARAM_SPEED_MIN,
	CONFIG_PARAM_DISTANCE_MIN,
	CONFIG_PARAM_MAX,
};

int config_module_init(void);
int config_param_write(enum config_param_type type, const void *data);
const void *config_param_get(enum config_param_type type);

#endif // __CONFIG_H_
