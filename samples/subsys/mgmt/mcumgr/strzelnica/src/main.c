/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Prevas A/S
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

#include <shell/shell.h>
#include "config.h"
#include "motor.h"
#include "common.h"
#include "encoder.h"
#include "led.h"
#include "buttons.h"

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

void buttons_cb(enum button_name name, enum button_event evt)
{
	if (name == BUTTON_NAME_CALL) {
		if (evt == BUTTON_EVT_PRESSED_SHORT) {
			led_blink_fast(LED_GREEN, 4);
		} else {
			led_blink_slow(LED_GREEN, 4);
		}
	} else if (name == BUTTON_NAME_SPEED) {
		if (evt == BUTTON_EVT_PRESSED_SHORT) {
			led_blink_fast(LED_RED, 4);
		} else {
			led_blink_slow(LED_RED, 4);
		}
	}
}

void main(void)
{
    int ret;
	int rc = STATS_INIT_AND_REG(smp_svr_stats, STATS_SIZE_32,
				    "smp_svr_stats");

	if (rc < 0) {
		LOG_ERR("Error initializing stats system [%d]", rc);
	}

	/* Register the built-in mcumgr command handlers. */
#ifdef CONFIG_MCUMGR_CMD_FS_MGMT
	rc = fs_mount(&littlefs_mnt);
	if (rc < 0) {
		LOG_ERR("Error mounting littlefs [%d]", rc);
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
		rc = usb_enable(NULL);
		if (rc) {
			LOG_ERR("Failed to enable USB");
			return;
		}
	}
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

	ret = buttons_init(buttons_cb);
	if (ret) {
		LOG_ERR("buttons module not initialized. err:%d", ret);
	}

	ret = config_module_init();
	if (ret) {
		LOG_ERR("config_module error (err: %d)", ret);
	}

//void)motor_init();
//otor_move(MOTOR_DRV_FORWARD, 3000);

  //bool forward = true;

	while (1) {
	buttons_enable(true);
	LOG_INF("Buttons ENABLED");
        k_sleep(K_MSEC(5000));
//        led_blink_fast(LED_GREEN, 10);
//        led_blink_fast(LED_RED, 10);
//        led_blink_fast(LED_BLUE, 10);
	buttons_enable(false);
	LOG_INF("Buttons disabled");
        k_sleep(K_MSEC(5000));

    }
#if 0
        int32_t samples;
		k_sleep(K_MSEC(1500));

        motor_move(MOTOR_DRV_BRAKE, 0);
		STATS_INC(smp_svr_stats, ticks);

        k_sleep(K_MSEC(500));
        if (forward) {
	        motor_move(MOTOR_DRV_BACKWARD, 7000);
        } else {
	        motor_move(MOTOR_DRV_FORWARD, 7000);
        }
        (void)encoder_get(&samples);
        forward = !forward;

	}
#endif
}

static int cmd_konf_read(const struct shell *shell, size_t argc, char **argv)
{
	const uint32_t *speed = config_param_get(CONFIG_PARAM_SPEED_MAX);

	shell_print(shell, "new value = %d", *speed);

	return 0;
}

static int cmd_konf_write(const struct shell *shell, size_t argc, char **argv)
{
	int rc;
	uint32_t data;
	uint32_t param;

	param = (int)strtol(argv[1], NULL, 10);
	data = (int)strtol(argv[2], NULL, 10);

	rc = config_param_write(param, &data);

	shell_print(shell, "rc = %d", rc);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_konf,
	SHELL_CMD(read, NULL, "Dictionary commands", cmd_konf_read),
	SHELL_CMD(write, NULL, "Hexdump params command.", cmd_konf_write),
    SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_ARG_REGISTER(konfig, &sub_konf, NULL, NULL, 2, 0);
