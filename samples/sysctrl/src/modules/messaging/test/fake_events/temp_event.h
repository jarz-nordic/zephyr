/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _TEMP_EVENT_H_
#define _TEMP_EVENT_H_

/**
 * @brief Temperature Service Event
 * @defgroup temp_event Temperature Service Event
 * @{
 */

#include "event_manager.h"

#include <internal/nrfs_phy.h>

#ifdef __cplusplus
extern "C" {
#endif

struct temp_event {
	struct event_header header;
	nrfs_phy_t *p_msg;
};

struct temp_event *new_temp_event(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _TEMP_EVENT_H_ */
