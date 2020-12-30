/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __ENCODER_H_
#define __ENCODER_H_

#include <zephyr/types.h>

struct encoder_result {
	int32_t acc;	// good samples (correct transitions)
	int32_t accdbl;	// bad samples (double transitions)
};

int encoder_init(bool trigger);

/* internal acumulators are cleared after read */
int encoder_get(struct encoder_result *data);

#endif // __ENCODER_H_
