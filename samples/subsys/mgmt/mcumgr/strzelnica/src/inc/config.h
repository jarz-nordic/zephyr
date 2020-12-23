
enum config_param_type {
	CONFIG_PARAM_SPEED_MAX,
	CONFIG_PARAM_SPEED_MIN,
	CONFIG_PARAM_DISTANCE_MIN,
	CONFIG_PARAM_MAX,
};

int config_module_init(void);
int config_param_write(enum config_param_type type, const void *data);
const void *config_param_get(enum config_param_type type);
