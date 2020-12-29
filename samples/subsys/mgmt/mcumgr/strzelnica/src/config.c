#include <zephyr.h>
#include <settings/settings.h>
#include <stdbool.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(config_module);

#include "config.h"

struct engine_cfg {
	uint32_t speed_max;
	uint32_t speed_min;
	uint32_t distance_min;
};
struct engine_cfg engine_cfg;
static bool initialized;

static int cfg_handle_set(const char *name, size_t len,
			  settings_read_cb read_cb, void *cb_arg)
{
	struct engine_cfg *p = &engine_cfg;
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
        return 0;
	}

	return -ENOENT;
}

static int cfg_handle_export(int (*cb)(const char *name,
			       const void *value, size_t val_len))
{
	const struct engine_cfg *p = &engine_cfg;

	LOG_INF("export keys under <CFG> handler");

	(void)cb("engine/cfg/speed_max", &p->speed_max, sizeof(p->speed_max));
	(void)cb("engine/cfg/speed_min", &p->speed_min, sizeof(p->speed_min));
	(void)cb("engine/cfg/distance_min", &p->distance_min,
		 sizeof(p->distance_min));

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

	LOG_WRN("max=%d|min=%d|dist=%d",
			engine_cfg.speed_max,
		       	engine_cfg.speed_min,
			engine_cfg.distance_min);

	rc = settings_subsys_init();
	if (rc != 0) {
		LOG_ERR("Initialization: fail (err %d)", rc);
		return rc;
	}

	rc = settings_load();

	if (rc == 0) {
		initialized = true;
	}
	LOG_WRN("max=%d|min=%d|dist=%d",
			engine_cfg.speed_max,
		       	engine_cfg.speed_min,
			engine_cfg.distance_min);

	return rc;
}

int config_param_write(enum config_param_type type, const void *const data)
{
	struct engine_cfg *p = &engine_cfg;
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
	default:
		LOG_INF("type is not valid");
		return -EINVAL;
	}

	return rc;
}

const void *config_param_get(enum config_param_type type)
{
	const struct engine_cfg *p = &engine_cfg;

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
	default:
		break;
	}

	LOG_INF("%s: Uknown parameter type", __FUNCTION__);
	return NULL;
}
