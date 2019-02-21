/* main.c - Bluetooth Cycling Speed and Cadence app main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/types.h>
#include <string.h>
#include <errno.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <logging/log.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <gatt/bas.h>

#include "peripherals.h"
#include "app_bt.h"

LOG_MODULE_REGISTER(app_main, LOG_LEVEL_DBG);


void main(void)
{
	int err;

	err = app_bt_init();
	if (err) {
		return;
	}

	err = peripherals_init();
	if (err) {
		return;
	}

	while (1) {
		k_sleep(MSEC_PER_SEC);

		app_bt_process();
	}
}
