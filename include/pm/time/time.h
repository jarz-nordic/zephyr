/*
 * Copyright (c) 2020 Nordic Semuconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @brief Time Management Module 
 */

#ifndef ZEPHYR_INCLUDE_PM_TIME_TIME_H_
#define ZEPHYR_INCLUDE_PM_TIME_TIME_H_

/**
 * @typedef pm_time_callback_fn
 * @brief Time Management callback function
 *
 * @param context Context used by higher layer.
 */
typedef void (*pm_time_callback_fn)(void *context);

/**
 * @brief Add callback at given time.
 *
 * This function adds a callback to the Time Management Module.
 * Function @p callback will be called at @p time with @p context.
 *
 * @param time     Time at which the callback shall be called.
 * @param callback Function that shall be called.
 * @param context  Pointer to additional context for higher layers. Should be cast.
 *
 * @retval 0 If successful.
 */

int pm_time_callback_add(uint64_t time, pm_time_callback_fn callback, void *context);

/**
 * @brief Get time used by Time Management Module.
 *
 * @return Current time from Time Management Module RTC.
 */
uint64_t pm_time_rtc_get(void);

#endif /* ZEPHYR_INCLUDE_PM_TIME_TIME_H_ */
