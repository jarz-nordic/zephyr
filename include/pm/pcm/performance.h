/*
 * Copyright (c) 2020 Nordic Semuconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @brief Power Control Module: Performance Requirement Translator
 */

#ifndef ZEPHYR_INCLUDE_PM_PCM_PERFORMANCE_H_
#define ZEPHYR_INCLUDE_PM_PCM_PERFORMANCE_H_

/**
 * @brief Transform performance request into power request.
 *
 * @param req Pointer to performance request to be turned into power request.
 *
 * @return Power request based on given performance request.
 */
struct pm_request_power pm_pcm_performance_into_power(struct pm_request_performance *req)
{
	struct pm_request_power pwr = {
		.domain   = req->domain,
		.time     = req->time,
		.mode     = req->mode,
		.resource = req->domain,
	};

	return pwr;
}

#endif /* ZEPHYR_INCLUDE_PM_PCM_PERFORMANCE_H_ */
