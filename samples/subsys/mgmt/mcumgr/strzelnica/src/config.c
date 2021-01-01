#include <zephyr.h>
#include <settings/settings.h>
#include <stdbool.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(config_module);

#include "config.h"

struct device_cfg {
	uint32_t speed_max;
	uint32_t speed_min;
	uint32_t distance_min;
	uint32_t distance_brake;
	uint32_t pid_kp;
	uint32_t pid_ki;
	uint32_t bt_pswd;
};


static struct device_cfg device_cfg;
static bool initialized;
static uint32_t crc;

static int cfg_handle_set(const char *name, size_t len,
			  settings_read_cb read_cb, void *cb_arg)
{
	struct device_cfg *p = &device_cfg;
	const char *next;
	size_t name_len;
	int rc;

	name_len = settings_name_next(name, &next);

	if (!next) {
		if (!strncmp(name, "speed_max", name_len)) {
			rc = read_cb(cb_arg, &p->speed_max,
				     sizeof(p->speed_max));
			return 0;
		}
		if (!strncmp(name, "speed_min", name_len)) {
			rc = read_cb(cb_arg, &p->speed_min,
				     sizeof(p->speed_min));
			return 0;
		}
		if (!strncmp(name, "distance_min", name_len)) {
			rc = read_cb(cb_arg, &p->distance_min,
				     sizeof(p->distance_min));
			return 0;
		}
		if (!strncmp(name, "distance_brake", name_len)) {
			rc = read_cb(cb_arg, &p->distance_brake,
				     sizeof(p->distance_brake));
			return 0;
		}
		if (!strncmp(name, "pid_kp", name_len)) {
			rc = read_cb(cb_arg, &p->pid_kp, sizeof(p->pid_kp));
			return 0;
		}
		if (!strncmp(name, "pid_ki", name_len)) {
			rc = read_cb(cb_arg, &p->pid_ki, sizeof(p->pid_ki));
			return 0;
		}
		if (!strncmp(name, "bt_pswd", name_len)) {
			rc = read_cb(cb_arg, &p->bt_pswd, sizeof(p->bt_pswd));
			return 0;
		}
		if (!strncmp(name, "crc", name_len)) {
			rc = read_cb(cb_arg, &crc, sizeof(crc));
			return 0;
		}
        return 0;
	}

	return -ENOENT;
}

static int cfg_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len))
{
	const struct device_cfg *p = &device_cfg;

	LOG_INF("export keys under <CFG> handler");

	(void)cb("engine/cfg/speed_max", &p->speed_max, sizeof(p->speed_max));
	(void)cb("engine/cfg/speed_min", &p->speed_min, sizeof(p->speed_min));
	(void)cb("engine/cfg/distance_min", &p->distance_min,
		 sizeof(p->distance_min));
	(void)cb("engine/cfg/distance_brake", &p->distance_brake,
		 sizeof(p->distance_brake));
	(void)cb("engine/cfg/pid_kp", &p->pid_kp, sizeof(p->pid_kp));
	(void)cb("engine/cfg/pid_ki", &p->pid_ki, sizeof(p->pid_ki));
	(void)cb("engine/cfg/bt_pswd", &p->bt_pswd, sizeof(p->bt_pswd));
	(void)cb("engine/cfg/crc", &crc, sizeof(crc));

	return 0;
}

static int cfg_handle_commit(void)
{
	LOG_INF("loading all settings under <cfg> handler is done");
	return 0;
}

static int cfg_handle_get(const char *name, char *val, int val_len_max)
{
	return -ENOENT;
}

/* static subtree handler */
SETTINGS_STATIC_HANDLER_DEFINE(cfg, "engine/cfg", cfg_handle_get,
			       cfg_handle_set, cfg_handle_commit,
			       cfg_handle_export);

int config_module_init(void)
{
	int rc;

	rc = settings_subsys_init();
	if (rc != 0) {
		LOG_ERR("Initialization: fail (err %d)", rc);
		return rc;
	}

	rc = settings_load();

	if (rc != 0) {
		LOG_ERR("Settings load fail (err %d)", rc);
		return rc;
	}

	if (crc == CONFIG_MAGIC_WORD) {
		initialized = true;
		LOG_INF("Settings loaded from flash.");
		return rc;
	}

	device_cfg.speed_max = CONFIG_DEFAULT_SPEED_MAX;
	device_cfg.speed_min = CONFIG_PARAM_SPEED_MIN;
	device_cfg.distance_min = CONFIG_PARAM_DISTANCE_MIN;
	device_cfg.distance_brake = CONFIG_DEFAULT_DISTANCE_BRAKE;
	device_cfg.pid_kp = CONFIG_DEFAULT_PID_KP;
	device_cfg.pid_ki = CONFIG_DEFAULT_PID_KI;
	device_cfg.bt_pswd = 253678;

	crc = CONFIG_MAGIC_WORD;

	rc = settings_save();

	if (rc) {
		LOG_INF("Cannot save default config in flash");
	} else {
		LOG_INF("Default settings saved");
	}

	return rc;
}

int config_param_write(enum config_param_type type, const void *const data)
{
	struct device_cfg *p = &device_cfg;
	int rc;

	if (!initialized) {
		return -EACCES;
	}

	if (!data) {
		LOG_INF("data is NULL pointer");
		return -EINVAL;
	}
	switch (type) {
	case CONFIG_PARAM_SPEED_MAX:
		p->speed_max = *((uint32_t *)data);
		rc = settings_save_one("engine/cfg/speed_max",
					(const void *)data,
					sizeof(uint32_t));
		LOG_INF("writing speed_max: %d", rc);
		break;
	case CONFIG_PARAM_SPEED_MIN:
		p->speed_min = *((uint32_t *)data);
		rc = settings_save_one("engine/cfg/speed_min",
					(const void *)data,
					sizeof(uint32_t));
		LOG_INF("writing speed_min: %d", rc);
		break;
	case CONFIG_PARAM_DISTANCE_MIN:
		p->distance_min = *((uint32_t *)data);
		rc = settings_save_one("engine/cfg/distance_min",
					(const void *)data,
					sizeof(uint32_t));
		LOG_INF("writing distance_min: %d", rc);
		break;
	case CONFIG_PARAM_DISTANCE_BRAKE:
		p->distance_brake = *((uint32_t *)data);
		rc = settings_save_one("engine/cfg/distance_brake",
					(const void *)data,
					sizeof(uint32_t));
		LOG_INF("writing distance_brake: %d", rc);
		break;
	case CONFIG_PARAM_PID_KP:
		p->pid_kp = *((uint32_t *)data);
		rc = settings_save_one("engine/cfg/pid_kp",
					(const void *)data,
					sizeof(uint32_t));
		LOG_INF("writing pid_kp: %d", rc);
		break;
	case CONFIG_PARAM_PID_KI:
		p->pid_ki = *((uint32_t *)data);
		rc = settings_save_one("engine/cfg/pid_ki",
					(const void *)data,
					sizeof(uint32_t));
		LOG_INF("writing pid_ki: %d", rc);
		break;
	case CONFIG_PARAM_BT_AUTH_PSWD:
		p->bt_pswd = *((uint32_t *)data);
		rc = settings_save_one("engine/cfg/bt_pswd",
					(const void *)data,
					sizeof(uint32_t));
		LOG_INF("writing bt_pswd: %d", rc);
		break;
	case CONFIG_CHECKSUM:
		crc = *((uint32_t *)data);
		rc = settings_save_one("engine/cfg/crc",
					(const void *)data,
					sizeof(uint32_t));
		LOG_INF("writing crc: %d", rc);
		break;
	default:
		LOG_INF("type is not valid");
		return -EINVAL;
	}

	return rc;
}

const void *config_param_get(enum config_param_type type)
{
	const struct device_cfg *p = &device_cfg;

	if (!initialized) {
		return NULL;
	}

	switch (type) {
	case CONFIG_PARAM_SPEED_MAX:
		return &p->speed_max;
	case CONFIG_PARAM_SPEED_MIN:
		return &p->speed_min;
	case CONFIG_PARAM_DISTANCE_MIN:
		return &p->distance_min;
	case CONFIG_PARAM_DISTANCE_BRAKE:
		return &p->distance_brake;
	case CONFIG_PARAM_PID_KP:
		return &p->pid_kp;
	case CONFIG_PARAM_PID_KI:
		return &p->pid_ki;
	case CONFIG_PARAM_BT_AUTH_PSWD:
		return &p->bt_pswd;
	case CONFIG_CHECKSUM:
		return &crc;
	default:
		break;
	}

	LOG_INF("%s: Uknown parameter type", __FUNCTION__);
	return NULL;
}
