/*
 * Copyright (c) 2020 Nordic Semuconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @brief Power Control Module: Global OFF Coordinator
 */

#ifndef ZEPHYR_INCLUDE_PM_PCM_GLOBAL_OFF_H_
#define ZEPHYR_INCLUDE_PM_PCM_GLOBAL_OFF_H_

/**
 * @brief Transform sleep request into power request.
 *
 * @param req Pointer to sleep request to be turned into power request.
 *
 * @return Power request based on given sleep request.
 */
struct pm_request_power pm_pcm_global_off_into_power(struct pm_request_sleep *req)
{
	/* TODO */
	struct pm_request_power pwr = {
		.domain   = req->domain,
		.time     = req->time,
		.mode     = (req->state == PM_SLEEP_RETENTION_SW)? 1 : 0,
		.resource = req->domain,
	};

	return pwr;
}

#endif /* ZEPHYR_INCLUDE_PM_PCM_GLOBAL_OFF_H_ */
