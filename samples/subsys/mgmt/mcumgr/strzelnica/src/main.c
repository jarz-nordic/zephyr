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

#include <settings/settings.h>
#include <shell/shell.h>

LOG_MODULE_REGISTER(smp_sample);

#include "common.h"
#define LED0_NODE DT_ALIAS(led0)
#define LED0    DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS   DT_GPIO_FLAGS(LED0_NODE, gpios)
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

static int speed;
int cfg_handle_set(const char *name, size_t len, settings_read_cb read_cb,
	void *cb_arg);
int cfg_handle_commit(void);
int cfg_handle_export(int (*cb)(const char *name,
		       const void *value, size_t val_len));
int cfg_handle_get(const char *name, char *val, int val_len_max);

void init_my_settings(void);
/* static subtree handler */
SETTINGS_STATIC_HANDLER_DEFINE(cfg, "engine/cfg", cfg_handle_get,
			       cfg_handle_set, cfg_handle_commit,
			       cfg_handle_export);


void main(void)
{

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
    const struct device *dev;
	bool led_is_on = true;
    int ret;

    LOG_ERR("start dev");
	dev = device_get_binding(LED0);
    LOG_ERR("step binded dev");

    if (dev) {
        LOG_ERR("LED dev found");
        ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
        if (ret < 0) {
            LOG_ERR("PIN not configured");
        }

    }
    init_my_settings();

    while (1) {
		k_sleep(K_MSEC(1000));
		STATS_INC(smp_svr_stats, ticks);
		gpio_pin_set(dev, PIN, (int)led_is_on);
		led_is_on = !led_is_on;
	}
}

static int cmd_konf_read(const struct shell *shell, size_t argc, char **argv)
{
    settings_load();
    shell_print(shell, "value = %d", speed);

    return 0;
}

static int cmd_konf_write(const struct shell *shell, size_t argc, char **argv)
{
    int rc;

    settings_load();

    speed = (int)strtol(argv[1], NULL, 10);

	rc = settings_save_one("engine/cfg/speed", (const void *)&speed,
			       sizeof(speed));
    settings_save();
    shell_print(shell, "rc = %d", rc);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_konf,
	SHELL_CMD(read, NULL, "Dictionary commands", cmd_konf_read),
	SHELL_CMD(write, NULL, "Hexdump params command.", cmd_konf_write),
    SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_ARG_REGISTER(konfig, &sub_konf, NULL, NULL, 2, 0);

int cfg_handle_set(const char *name, size_t len, settings_read_cb read_cb,
		  void *cb_arg)
{
	const char *next;
	size_t name_len;
	int rc;

	name_len = settings_name_next(name, &next);

	if (!next) {
		if (!strncmp(name, "speed", name_len)) {
			rc = read_cb(cb_arg, &speed, sizeof(speed));
			printk("<engine/cfg/speed> = %d\n", speed);
			return 0;
		}

        return 0;
	}

	return -ENOENT;
}

int cfg_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len))
{
	printk("export keys under <CFG> handler\n");
	(void)cb("engine/cfg/speed", &speed, sizeof(speed));

	return 0;
}

int cfg_handle_commit(void)
{
	printk("loading all settings under <cfg> handler is done\n");
	return 0;
}

int cfg_handle_get(const char *name, char *val, int val_len_max)
{
	const char *next;

	return -ENOENT;
}

void init_my_settings(void)
{
    int rc;

	rc = settings_subsys_init();
	if (rc) {
		printk("settings subsys initialization: fail (err %d)\n", rc);
		return;
	}

	settings_load();
}

