/*
 * Copyright (c) 2020 Nordic Semuconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @brief Power Control Module: Power Tree Algorithm
 */

#ifndef ZEPHYR_INCLUDE_PM_PCM_ALGORITHM_H_
#define ZEPHYR_INCLUDE_PM_PCM_ALGORITHM_H_

#include "pm/pm.h"

/**
 * @brief Run Power Tree Algorithm.
 *
 * This function runs Power Tree Algorithm, based on given @p state
 * and requested @p mode of given @p resource. If @p enforce is set
 * to true, while traversing the tree, it will use drivers to enforce
 * necessary adjustments.
 *
 * @param state    Pointer to state before the requested change.
 * @param resoruce Resource that will be changed.
 * @param mode     Mode that the resource should be changed to.
 * @param enforce  Use drivers to enforce adjustments while traversing.
 *
 * @return Traversal structure, containing information about possibility
 *         of requested change, time needed and subsequent state.
 */
struct pm_traversal pm_pcm_algorithm_run(struct pm_state *state, u8_t resource,
					 u8_t mode, bool enforce);

#endif /* ZEPHYR_INCLUDE_PM_PCM_ALGORITHM_H_ */
