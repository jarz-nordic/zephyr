/*
 * Copyright (c) 2020 Nordic Semuconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include "pm/pm.h"
#include "pm/pcm/dispatcher.h"
#include "pm/pcm/scheduler.h"
#include "pm/pcm/algorithm.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(PM_PCM_DISPATCHER, 4);

static struct pm_state state_now;

void pm_pcm_dispatcher_init(void)
{
	/* Set all resources to maximum mode at startup. */
	for(uint8_t i = 0; i < PM_RESOURCE_NUM; i++)
	{
		state_now.res[i] = PM_MODE_NUM - 1;
	}
}

struct pm_return pm_pcm_dispatcher_event_request(enum pm_pcm_dispatcher_event evt_type,
						 void * ctx)
{
	struct pm_return ret;
	if (evt_type == PM_PCM_DISPATCHER_EVENT_POWER)
	{
		struct pm_request_power *req = (struct pm_request_power *)ctx;
		ret = pm_pcm_dispatcher_traversal_request(req);

		/* Only immediate and impossible responses need to return now, schedule rest */
		if ((req->time != 0) && (ret.response != PM_RESPONSE_IMPOSSIBLE))
		{
			/* TODO: what if curr_time - ret.time is too small to schedule? */
			if(pm_pcm_scheduler_return_delayed(&ret, req->time) != 0)
			{
				LOG_ERR("Unable to schedule!");
				ret.response = PM_RESPONSE_ERROR;
				return ret;
			}
			ret.response = PM_RESPONSE_NONE;
		}
	}
	else
	{
		/* TODO */
		ret.response = PM_RESPONSE_IMPOSSIBLE;
		//ret.domain = req->domain;
	}
	return ret;
}

#if 0 /* TODO: implement traversal */
void pm_pcm_dispatcher_traversal_requirement_add(struct pm_request_power *req)
{
	/* TODO: add to linked list in time order */
}

struct pm_request_power pm_pcm_dispatcher_traversal_requirement_last_get(void)
{
	/* TODO: return last element from linked list */
}

struct pm_state pm_pcm_dispatcher_traversal_state_at(u64_t time)
{
	/* TODO: get state from future controller */
}
#endif

struct pm_return pm_pcm_dispatcher_traversal_request(struct pm_request_power *req)
{
	struct pm_return ret = {
		.response = PM_RESPONSE_OK,
		.domain = req->domain,
		.resource = req->resource,
		.mode = req->mode,
	};
#if 0
	/* TODO: whole traversal algo */
	pm_pcm_dispatcher_traversal_requirement_add(req);

	pm_request_power *new = pm_pcm_dispatcher_traversal_requirement_last_get();
	pm_request_power *old = new->prev;
	for (uint8_t i = pm_pcm_dispatcher_traversal_requirement_len() - 1; i > 0; i--)
	{
		u64_t time_delta = new->time - old->time;
		struct pm_state state_before = pm_pcm_dispatcher_traversal_state_at(old->time);


		struct pm_traversal trav = pm_pcm_algorithm_run(&state_before, new->resource,
								new->mode, false);
		if (trav.time >= time_delta)
		{

		}

		old = new;
		new = new->prev;
	}
	/* Foreach difference between states */
	/* Then take maximum */
#endif


	return ret;
}
