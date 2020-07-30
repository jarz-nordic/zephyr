/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _GPMS_NOTIFY_EVENT_H_
#define _GPMS_NOTIFY_EVENT_H_

/**
 * @brief GPMS_NOTIFY Event
 * @defgroup gpms_notify_event GPMS_NOTIFY Event
 * @{
 */

#include "event_manager.h"

#include <internal/nrfs_phy.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gpms_notify_event {
    struct event_header header;
    uint32_t            ctx;
    void              * p_msg;
    size_t              msg_size;
    int32_t             status;
};

EVENT_TYPE_DECLARE(gpms_notify_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _GPMS_NOTIFY_EVENT_H_ */
