/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _GPMS_H_
#define _GPMS_H_



/* @brief Function for notifying GPMS.
 *
 * Send notification to a domain specified in NCM context.
 *
 * @param p_resp    Pointer to the service response structure.
 * @param resp_size Size of the service response structure.
 * @param p_ctx     Pointer to the NCM context.
 *
 * @retval 0       Notification sent with success.
 *         -ENOMEM Failed to allocate memory for the message.
 */
int gpms_notify(void *p_resp, size_t resp_size, struct ncm_ctx * p_ctx);


/* @brief Function responding to GPMS.
 *
 * Send response for request with given GPMS context.
 *
 * @param p_resp    Pointer to the service response structure.
 * @param resp_size Size of the service response structure.
 * @param ctx       GPMS context.
 * @param result    Result of the operation.
 *
 * @retval 0       Notification sent with success.
 *         -ENOMEM Failed to allocate memory for the message.
 */
int gpms_response(void *p_resp, size_t resp_size, uint32_t gpms_ctx, int32_t result);

#endif /* _GPMS_H_ */
