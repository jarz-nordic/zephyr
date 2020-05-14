/*
 * Copyright (c) 2020 Nordic Semuconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>

#include "pm/pm.h"
#include "pm/pcm/scheduler.h"
#include "pm/pcm/dispatcher.h"
#include "pm/time/time.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(PM_PCM_SCHEDULER, 4);

#define PM_PCM_SCHEDULER_RETURN_NUM 255 /*TODO: move to pm.h ?*/

static struct pm_pcm_scheduler_return scheduled_list[PM_PCM_SCHEDULER_RETURN_NUM];

static inline int pm_pcm_scheduler_scheduled_return_acquire(u32_t *id)
{
	for(u32_t i = 0; i < PM_PCM_SCHEDULER_RETURN_NUM; i++)
	{
		if (k_sem_take(&scheduled_list[i].semaphore, K_NO_WAIT) == 0)
		{
			*id = scheduled_list[i].id;
			LOG_INF("Returning scheduled return nr %u", *id);
			return 0;
		}
	}
	return -ENOMEM;
}

static inline void pm_pcm_scheduler_scheduled_return_free(u32_t id)
{
	k_sem_give(&scheduled_list[id].semaphore);
}

void time_callback(void *context)
{
	u32_t *id = (u32_t *)context;

	(void)pm_pcm_dispatcher_event_request(PM_PCM_DISPATCHER_EVENT_DELAYED, &scheduled_list->ret);

	pm_pcm_scheduler_scheduled_return_free(*id);
}

int pm_pcm_scheduler_return_delayed(struct pm_return *ret, u64_t time)
{
	u32_t id;
	if (pm_pcm_scheduler_scheduled_return_acquire(&id) != 0)
	{
		LOG_ERR("Unable to acquire scheduled return");
		return -ENOMEM;
	}

	memcpy(&scheduled_list[id].ret, ret, sizeof(struct pm_return));

	pm_time_callback_add(time, time_callback, &id);

	return 0;
}

void pm_pcm_scheduler_init(void)
{
	for(u32_t i = 0; i < PM_PCM_SCHEDULER_RETURN_NUM; i++)
	{
		k_sem_init(&scheduled_list[i].semaphore, 1, 1);
		scheduled_list[i].id = i;
	}
}

