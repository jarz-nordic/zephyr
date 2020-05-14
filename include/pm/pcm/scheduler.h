/*
 * Copyright (c) 2020 Nordic Semuconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @brief Power Control Module: Power Control Scheduler
 */

#ifndef ZEPHYR_INCLUDE_PM_PCM_SCHEDULER_H_
#define ZEPHYR_INCLUDE_PM_PCM_SCHEDULER_H_

/**
 * @brief Scheduled return object.
 * Used to organize scheduled returns.
 */
struct pm_pcm_scheduler_return {
	u32_t            id;
	struct k_sem     semaphore;
	struct pm_return ret;
};

/** @brief Initialize Power Control Dispatcher. */
void pm_pcm_scheduler_init(void);

/**
 * @brief Scheduled delayed return.
 *
 * This function adds return to scheduled return objects.
 * It will trigger that return based on @p time.
 *
 * @param ret  Pointer to return structure to be scheduled.
 * @param time Time at which the return should be triggered.
 *
 * @retval 0       If successful.
 * @retval -ENOMEM Unable to acquire scheduled return object.
 */

int pm_pcm_scheduler_return_delayed(struct pm_return *ret, u64_t time);

#endif /* ZEPHYR_INCLUDE_PM_PCM_SCHEDULER_H_ */
