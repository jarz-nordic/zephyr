/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ENCODER_H_
#define __ENCODER_H_

#include <zephyr/types.h>

int encoder_init(void);
int encoder_get(int32_t *data);

#endif // __ENCODER_H_
