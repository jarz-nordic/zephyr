/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _MTS_EVENT_H_
#define _MTS_EVENT_H_

/**
 * @brief Memory Transfer Service Event
 * @defgroup mts_event Memory Transfer Service Event
 * @{
 */

#include "event_manager.h"

#include <internal/nrfs_phy.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mts_event {
	struct event_header header;
	nrfs_phy_t *p_msg;
};

struct mts_event *new_mts_event(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MTS_EVENT_H_ */
