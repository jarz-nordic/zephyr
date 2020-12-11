/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _GPMS_EVENT_H_
#define _GPMS_EVENT_H_

/**
 * @brief GPMS Events
 * @defgroup gpms_events GPMS Events
 * @{
 */

#include "event_manager.h"

#include <internal/nrfs_phy.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gpms_event {
    struct event_header header;
    nrfs_phy_t        * p_msg;
};

struct gpms_response_event {
    struct event_header header;
    uint32_t            ctx;
    void              * p_msg;
    size_t              msg_size;
    int32_t             status;
};

struct gpms_notify_event {
    struct event_header header;
    struct ncm_ctx    * p_ctx;
    void              * p_msg;
    size_t              msg_size;
};

struct gpms_event *new_gpms_event(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _GPMS_EVENT_H_ */
