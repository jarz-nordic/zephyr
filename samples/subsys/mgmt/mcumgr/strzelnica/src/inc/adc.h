/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ADC_H_
#define __ADC_H_

#include <zephyr/types.h>

int adc_init(void);
int adc_get(int32_t *result);

#endif // __ENCODER_H_
