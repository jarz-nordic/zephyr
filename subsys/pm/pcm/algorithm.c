/*
 * Copyright (c) 2020 Nordic Semuconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include "pm/pm.h"
#include "pm/pcm/algorithm.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(PM_PCM_ALGORITHM, 4);

struct pm_traversal pm_pcm_algorithm_run(struct pm_state *state, uint8_t resource,
					 uint8_t mode, bool enforce)
{
	__ASSERT_NO_MSG(resource < PM_RESOURCE_NUM);
	__ASSERT_NO_MSG(mode < PM_MODE_NUM);

	struct pm_traversal traversal;

	memcpy(&traversal.state, state, sizeof(struct pm_state));

	if (state->res[resource] > mode) {
		traversal.response = PM_RESPONSE_OK;
		traversal.time = 100;
	} else if (state->res[resource] == mode) {
		traversal.response = PM_RESPONSE_OK;
		traversal.time = 0;
	} else {
		traversal.response = PM_RESPONSE_OK;
		traversal.time = 10;
	}

	traversal.state.res[resource] = mode;

	return traversal;
}
