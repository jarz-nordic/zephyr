/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Prevas A/S
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <zephyr.h>
#include <stats/stats.h>
#include <usb/usb_device.h>

#include <devicetree.h>
#include <drivers/gpio.h>

#ifdef CONFIG_MCUMGR_CMD_FS_MGMT
#include <device.h>
#include <fs/fs.h>
#include "fs_mgmt/fs_mgmt.h"
#include <fs/littlefs.h>
#endif
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
#include "os_mgmt/os_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include "img_mgmt/img_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_STAT_MGMT
#include "stat_mgmt/stat_mgmt.h"
#endif

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>

#include "config.h"
#include "motor.h"
#include "common.h"
#include "led.h"
#include "state_machine.h"

LOG_MODULE_REGISTER(main);


/* Define an example stats group; approximates seconds since boot. */
STATS_SECT_START(smp_svr_stats)
STATS_SECT_ENTRY(ticks)
STATS_SECT_END;

/* Assign a name to the `ticks` stat. */
STATS_NAME_START(smp_svr_stats)
STATS_NAME(smp_svr_stats, ticks)
STATS_NAME_END(smp_svr_stats);

/* Define an instance of the stats group. */
STATS_SECT_DECL(smp_svr_stats) smp_svr_stats;

#ifdef CONFIG_MCUMGR_CMD_FS_MGMT
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);
static struct fs_mount_t littlefs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &cstorage,
	.storage_dev = (void *)FLASH_AREA_ID(storage),
	.mnt_point = "/lfs"
};
#endif


void main(void)
{
	int ret;

	/* using __TIME__ ensure that a new binary will be built on every
	 * compile which is convient when testing firmware upgrade.
	 */
	LOG_INF("build time: " __DATE__ " " __TIME__);

	/* The system work queue handles all incoming mcumgr requests.  Let the
	 * main thread idle while the mcumgr server runs.
	 */
	ret = led_thread_init();
	if (ret) {
		LOG_ERR("led module not initialized. err:%d", ret);
	}

	ret = config_module_init();
	if (ret) {
		LOG_ERR("config_module error (err: %d)", ret);
	}

	ret = STATS_INIT_AND_REG(smp_svr_stats, STATS_SIZE_32,
	 			 "smp_svr_stats");

	if (ret < 0) {
		led_blink_slow(LED_RED, 2);
		LOG_ERR("Error initializing stats system [%d]", ret);
	}

	/* Register the built-in mcumgr command handlers. */
#ifdef CONFIG_MCUMGR_CMD_FS_MGMT
	ret = fs_mount(&littlefs_mnt);
	if (ret < 0) {
		led_blink_slow(LED_RED, 3);
		LOG_ERR("Error mounting littlefs [%d]", ret);
	}

	fs_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
	os_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
	img_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_STAT_MGMT
	stat_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_SMP_BT
	start_smp_bluetooth();
#endif
#ifdef CONFIG_MCUMGR_SMP_UDP
	start_smp_udp();
#endif

	if (IS_ENABLED(CONFIG_USB)) {
		ret = usb_enable(NULL);
		if (ret) {
			led_blink_slow(LED_RED, 4);
			LOG_ERR("Failed to enable USB");
			return;
		}
	}

	ret = motor_init();
	if (ret) {
		LOG_ERR("motor init error (err: %d)", ret);
		led_blink_slow(LED_RED, LED_BLINK_INFINITE);
		return;
	}

	ret = state_machine_init();
	if (ret) {
		LOG_ERR("State machine init error (err: %d)", ret);
	}
}
