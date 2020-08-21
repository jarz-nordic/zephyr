/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _RADIO_EVENT_H_
#define _RADIO_EVENT_H_

/**
 * @brief RADIO Event
 * @defgroup radio_event RADIO Event
 * @{
 */

#include "event_manager.h"

#include <nrfs_phy.h>

#ifdef __cplusplus
extern "C" {
#endif

struct radio_event {
    struct event_header header;
    nrfs_phy_t *p_msg;
    int32_t status;
};

EVENT_TYPE_DECLARE(radio_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _RADIO_EVENT_H_ */
