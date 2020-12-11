/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _CLOCK_EVENT_H_
#define _CLOCK_EVENT_H_

/**
 * @brief CLOCK Event
 * @defgroup clock_event CLOCK Event
 * @{
 */

#include "event_manager.h"

#include <internal/nrfs_phy.h>

#ifdef __cplusplus
extern "C" {
#endif

struct clock_event {
    struct event_header header;
    nrfs_phy_t *p_msg;
    int32_t status;
};

struct clock_power_event {
    struct event_header header;
    nrfs_phy_t *p_msg;
};

struct clock_done_event {
    struct event_header header;
    nrfs_phy_t *p_msg;
};

struct clock_source_event {
    struct event_header header;
    nrfs_phy_t *p_msg;
    int32_t status;
};

struct clock_event *new_clock_event(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _CLOCK_EVENT_H_ */
