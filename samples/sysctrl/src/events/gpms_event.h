/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _GPMS_EVENT_H_
#define _GPMS_EVENT_H_

/**
 * @brief GPMS Event
 * @defgroup gpms_event GPMS Event
 * @{
 */

#include "event_manager.h"

#include <nrfs_phy.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gpms_event {
    struct event_header header;
    nrfs_phy_t        * p_msg;
};

EVENT_TYPE_DECLARE(gpms_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _GPMS_EVENT_H_ */
