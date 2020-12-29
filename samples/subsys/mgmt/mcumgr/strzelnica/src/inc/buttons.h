/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BUTTONS_H_
#define __BUTTONS_H_

#include <zephyr/types.h>

enum button_event {
	BUTTON_EVT_PRESSED_SHORT,
	BUTTON_EVT_PRESSED_LONG,
	BUTTON_EVT_STUCKED,
};

enum button_name {
	BUTTON_NAME_SPEED,
	BUTTON_NAME_CALL,
	/* internal value: */
	BUTTON_NAME_MAX,
};

typedef void (* button_hander_t)(enum button_name name, enum button_event evt);

int buttons_init(button_hander_t button_cb);
int buttons_enable(bool enable);
int button_enable(enum button_name name, bool enable);

#endif // __BUTTONS_H_
