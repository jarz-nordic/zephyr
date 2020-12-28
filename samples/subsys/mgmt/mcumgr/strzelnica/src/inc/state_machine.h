/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __STATE_MACHINE_H_
#define __STATE_MACHINE_H_

#include <zephyr/types.h>

int encoder_init(void);
int encoder_get(int32_t *data);

#endif
