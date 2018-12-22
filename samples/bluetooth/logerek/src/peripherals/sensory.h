/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SENSORY_H__
#define SENSORY_H__

#define INVALID_SENSOR_VALUE	(s32_t)(-500000)

int sensory_init(void);
int sensory_get_temperature(void);
int sensory_get_humidity(void);
void sensory_set_temperature_external(s16_t tmp);

#endif

