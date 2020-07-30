/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _PRISM_EVENT_H_
#define _PRISM_EVENT_H_

/**
 * @brief PRISM Event
 * @defgroup prism_event PRISM Event
 * @{
 */

#include "event_manager.h"

#include <internal/nrfs_phy.h>

#ifdef __cplusplus
extern "C" {
#endif

enum prism_msg_status {
	PRISM_MSG_STATUS_RX,
	PRISM_MSG_STATUS_RX_RELEASED,
};

struct prism_event {
	struct event_header header;
	nrfs_phy_t *p_msg;
	enum prism_msg_status status;
};

/**
 * @brief Sends prism event to the event manager, setting status to
 *	  PRISM_MSG_STATUS_RX_RELEASED.
 *
 * @param msg	Pointer to the message with request
 */
void prism_event_release(nrfs_phy_t *msg);

EVENT_TYPE_DECLARE(prism_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _PRISM_EVENT_H_ */
