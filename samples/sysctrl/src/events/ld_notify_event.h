/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _LD_NOTIFY_EVENT_H_
#define _LD_NOTIFY_EVENT_H_

/**
 * @brief Local Domain Notification Event
 * @defgroup ld_notify_event Local Domain Notification Event
 * @{
 */

#include "event_manager.h"
#include "ncm.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ld_notify_event {
	struct event_header header;
	nrfs_phy_t msg;
};

EVENT_TYPE_DECLARE(ld_notify_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _LD_NOTIFY_EVENT_H_ */
