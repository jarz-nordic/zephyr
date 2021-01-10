/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr.h>
#include <stats/stats.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/gatt.h>

#include "led.h"
#include "buttons.h"
#include "encoder.h"
#include "motor.h"
#include "pid.h"
#include "config.h"
#include "adc.h"

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>
LOG_MODULE_REGISTER(state_machine);

#define THREAD_STACK_SIZE	2048
#define THREAD_PRIORITY		K_PRIO_PREEMPT(5)

#define ONE_ROTATION_PULSES		(64u)
#define SM_SPEED_SAMPLE_MS		(K_MSEC(100u))
#define SM_SPEED_FIRST_SAMPLE_MS	(K_MSEC(200u))

#define PULSES_MOTOR_STOP_THRESHOLD	4

enum sm_states {
	SM_INIT,
	SM_CHECK_LICENSE,
	SM_IDLE,
	SM_CALIBRATION_RUN,
	SM_RUNNING,
	SM_ERROR,
	SM_MAX
};

struct drive_info {
	int32_t length;		// last measured track length
	int32_t position;	// current position
	int32_t distance_brake;	// position where motor must slowing down
	int32_t distance_slow;	// position where motor must run very slow
	uint32_t speed_max;
	uint32_t speed_min;
	uint32_t speed_brake;
};

static enum sm_states state;
static struct drive_info lap;
static bool button_pressed;
static K_THREAD_STACK_DEFINE(thread_stack, THREAD_STACK_SIZE);
static struct k_thread thread;
static struct k_mutex state_mtx;

static bool is_motor_stopped(int32_t val, uint32_t filter)
{
	static uint32_t cnt = 0;

	if (abs(val) <= PULSES_MOTOR_STOP_THRESHOLD) {
		cnt++;
	} else {
		cnt = 0;
	}

	return cnt > filter;
}

static void update_lap_len(struct drive_info *info)
{
	const uint32_t *min_len;

	min_len = (const uint32_t *)config_param_get(CONFIG_PARAM_DISTANCE_MIN);

	if (abs(info->position) >= *min_len) {
		info->length = info->position;
		LOG_INF("%s: %d", __FUNCTION__, info->length);
	}
}

static inline bool is_distance_slow(const struct drive_info *info)
{
	/* oposite direction so len and position have different sign */
	return abs(info->length + info->position) < info->distance_slow;
}

static inline bool is_distance_brake(const struct drive_info *info)
{
	if (abs(info->length) <= abs(info->position)) {
		return true;
	}

	/* oposite direction so len and position have different sign */
	return abs(info->length + info->position) < info->distance_brake;
}

static bool is_overcurrent(void)
{
	int current;
	int rc;

	rc = adc_get(&current);

	LOG_INF(" => current = %d", current);

	return false;
}

static void run_motor(struct drive_info *const info,
		      const enum motor_drv_direction direction,
		      uint32_t power, const uint32_t stop_samples)
{
	struct encoder_result sensor;
	pid_data_t pid = {
		.max_val = MOTOR_PWM_MAX_PERIOD_US,
		.min_val = MOTOR_PWM_MIN_PERIOD_US,
		.set_value = info->speed_max,
	};

	PID_Init(); // reset PID
	motor_move(direction, power);
	k_sleep(SM_SPEED_FIRST_SAMPLE_MS); // wait for motor start
	encoder_get(&sensor); // reset sensor readout

	while (1) {
		k_sleep(SM_SPEED_SAMPLE_MS);

		encoder_get(&sensor);
		pid.measured_value = abs(sensor.acc);
		info->position += sensor.acc;

		if (is_distance_brake(info)) {
			pid.set_value = info->speed_brake;
		} else if (is_distance_slow(info)) {
			pid.set_value = info->speed_min;
		}

		power = PID_Controller(&pid);

		motor_move(direction, power);

		if (is_motor_stopped(sensor.acc, stop_samples) ||
		    is_overcurrent()) {
			power = 0;
			motor_move(MOTOR_DRV_NEUTRAL, power);
			return;
		}

	}
}

void buttons_cb(enum button_name name, enum button_event evt)
{
	if (name == BUTTON_NAME_CALL) {
		if (evt == BUTTON_EVT_PRESSED_SHORT) {
			led_blink_fast(LED_GREEN, 3);
		} else {
			led_blink_slow(LED_GREEN, 3);
		}
		button_enable(BUTTON_NAME_CALL, true);
	} else if (name == BUTTON_NAME_SPEED) {
		if (evt == BUTTON_EVT_PRESSED_SHORT) {
			led_blink_fast(LED_RED, 3);
		} else {
			led_blink_slow(LED_RED, 3);
		}
		button_enable(BUTTON_NAME_SPEED, true);
	}

	button_pressed = true;
}

static inline void state_set(enum sm_states new_state)
{
	if (state < SM_MAX) {
		k_mutex_lock(&state_mtx, K_FOREVER);
		state = new_state;
		k_mutex_unlock(&state_mtx);
	}
}

static bool is_lap_valid(const struct drive_info *info)
{
	const uint32_t *ptr;

	ptr = config_param_get(CONFIG_PARAM_DISTANCE_MIN);

	return abs(info->length) >= *ptr;
}

static inline void switch_driection(enum motor_drv_direction *dir)
{
	*dir = *dir == MOTOR_DRV_FORWARD ? MOTOR_DRV_BACKWARD :
					   MOTOR_DRV_FORWARD;
}

static void lap_prepare(struct drive_info *lap, bool first_run)
{
	const uint32_t *ptr;

	lap->position = 0;

	if (first_run) {
		lap->length = 0xFFFFFFFF;
		lap->distance_brake = 0; // constant speed at first run
		lap->distance_slow = 0; // constant speed at first run
		ptr = config_param_get(CONFIG_PARAM_SPEED_MIN);
		lap->speed_max = *ptr;
		lap->speed_min = *ptr;
		lap->speed_brake = *ptr;
	} else {
		ptr = config_param_get(CONFIG_PARAM_DISTANCE_BRAKE);
		lap->distance_brake = *ptr;
		ptr = config_param_get(CONFIG_PARAM_DISTANCE_SLOW);
		lap->distance_slow = *ptr;
		ptr = config_param_get(CONFIG_PARAM_SPEED_MAX);
		lap->speed_max = *ptr;
		ptr = config_param_get(CONFIG_PARAM_SPEED_MIN);
		lap->speed_min = *ptr;
		ptr = config_param_get(CONFIG_PARAM_SPEED_BRAKE);
		lap->speed_brake = *ptr;
	}
}

static void lap_finish(struct drive_info *lap,
		       enum motor_drv_direction *direction)
{
	update_lap_len(lap);

	if (is_lap_valid(lap)) {
		LOG_DBG("lap valid");
		state_set(SM_IDLE);
		buttons_enable(true);
	} else {
		state_set(SM_CALIBRATION_RUN);
	}

	switch_driection(direction);
}

static void state_machine_thread_fn(void)
{
	enum motor_drv_direction direction = MOTOR_DRV_FORWARD;
	uint32_t stop_samples;
	const uint32_t *ptr;
	uint32_t power;
	int ret;


	while(1) {
		switch (state) {
		case SM_INIT:
			ret = encoder_init(false);
			LOG_INF("encoder init: [%s]",
				ret != 0 ? "error" : "ok");
			if (ret) {
				LOG_ERR("  error: [%d]", ret);
				led_blink_fast(LED_RED, LED_BLINK_INFINITE);
				state_set(SM_ERROR);
				return;
			}
			state_set(SM_CHECK_LICENSE);
			break;
		case SM_CHECK_LICENSE:
			LOG_INF("checking license");
			config_print_params();
			state_set(SM_CALIBRATION_RUN);
			break;
		case SM_CALIBRATION_RUN:
			lap_prepare(&lap, true);
			ptr = config_param_get(CONFIG_PARAM_START_POWER);
			power = *ptr;
			stop_samples = 10;

			run_motor(&lap, direction, power, stop_samples);

			lap_finish(&lap, &direction);
			break;
		case SM_IDLE:
			if (button_pressed) {
				state_set(SM_RUNNING);
				buttons_enable(false);
				button_pressed = false;
				break;
			}
			break;
		case SM_RUNNING:
			lap_prepare(&lap, false);
			ptr = config_param_get(CONFIG_PARAM_START_POWER);
			power = *ptr;
			stop_samples = 3;

			run_motor(&lap, direction, power, stop_samples);

			lap_finish(&lap, &direction);
			break;
		case SM_ERROR:
			/* do nothing */
			break;
		default:
			led_blink_slow(LED_RED, LED_BLINK_INFINITE);
			LOG_ERR("%s: unexpected SM state.", __FUNCTION__);
			break;
		}

		k_sleep(K_MSEC(500));
	}
}

int state_machine_init(void)
{
	int ret;

	ret = buttons_init(buttons_cb);
	LOG_INF("buttons init: [%s]", ret != 0 ? "error" : "ok");
	if (ret) {
		LOG_ERR("  error: %d", ret);
		return ret;
	}

	buttons_enable(true);
	k_mutex_init(&state_mtx);

	k_thread_create(&thread, thread_stack,
			THREAD_STACK_SIZE,
			(k_thread_entry_t)state_machine_thread_fn,
			NULL, NULL, NULL,
			THREAD_PRIORITY, 0, K_MSEC(100));
	k_thread_name_set(&thread, "state_machine_thread");

	return 0;
}

static ssize_t bt_start_lap(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr, const void *buf,
			    uint16_t len, uint16_t offset, uint8_t flags)
{
	const char *msg;

	if (state != SM_IDLE) {
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	msg = buf;

	if (*msg == 1) {
		led_blink_fast(LED_BLUE, 2);
		state_set(SM_RUNNING);
		buttons_enable(false);
		button_pressed = false;
	}

	return 0;
}

static ssize_t bt_show_state(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	const char *lookup[] = {"Init", "Check License", "idle",
				"calibration_run", "running", "error"};
	const char *value = lookup[state];

	led_blink_fast(LED_BLUE, 2);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, value,
				 strlen(value));
}

static ssize_t bt_show_lap_length(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf, uint16_t len, uint16_t offset)
{
	int32_t val = lap.length;

	led_blink_fast(LED_BLUE, 2);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, &val,
				 sizeof(val));
}

static ssize_t bt_get_speed_max(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	const uint32_t *val;

	val = (const uint32_t *)config_param_get(CONFIG_PARAM_SPEED_MAX);

	led_blink_fast(LED_BLUE, 2);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, val,
				 sizeof(val));
}

static ssize_t bt_set_speed_max(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	const uint32_t *msg;

	if (k_mutex_lock(&state_mtx, K_NO_WAIT)) {
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	if (state != SM_IDLE) {
		k_mutex_unlock(&state_mtx);
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	msg = (const uint32_t *)buf;
	config_param_write(CONFIG_PARAM_SPEED_MAX, msg);
	k_mutex_unlock(&state_mtx);
	LOG_INF("bt speed_max = %d", *msg);
	led_blink_fast(LED_BLUE, 2);

	return 0;
}

static ssize_t bt_get_speed_min(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	const uint32_t *val;

	val = (const uint32_t *)config_param_get(CONFIG_PARAM_SPEED_MIN);
	led_blink_fast(LED_BLUE, 2);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, val,
				 sizeof(val));
}

static ssize_t bt_set_speed_min(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	const uint32_t *msg;

	if (k_mutex_lock(&state_mtx, K_NO_WAIT)) {
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	if (state != SM_IDLE) {
		k_mutex_unlock(&state_mtx);
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	msg = (const uint32_t *)buf;
	config_param_write(CONFIG_PARAM_SPEED_MIN, msg);
	k_mutex_unlock(&state_mtx);
	LOG_INF("bt speed_min = %d", *msg);
	led_blink_fast(LED_BLUE, 2);

	return 0;
}

static ssize_t bt_get_speed_brake(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf, uint16_t len, uint16_t offset)
{
	const uint32_t *val;

	val = (const uint32_t *)config_param_get(CONFIG_PARAM_SPEED_BRAKE);
	led_blink_fast(LED_BLUE, 2);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, val,
				 sizeof(val));
}

static ssize_t bt_set_speed_brake(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  const void *buf, uint16_t len,
				  uint16_t offset, uint8_t flags)
{
	const uint32_t *msg;

	if (k_mutex_lock(&state_mtx, K_NO_WAIT)) {
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	if (state != SM_IDLE) {
		k_mutex_unlock(&state_mtx);
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	msg = (const uint32_t *)buf;
	config_param_write(CONFIG_PARAM_SPEED_BRAKE, msg);
	k_mutex_unlock(&state_mtx);
	LOG_INF("bt speed_brake = %d", *msg);
	led_blink_fast(LED_BLUE, 2);

	return 0;
}

static ssize_t bt_get_distance_slow(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr,
				    void *buf, uint16_t len, uint16_t offset)
{
	const uint32_t *val;

	val = (const uint32_t *)config_param_get(CONFIG_PARAM_DISTANCE_SLOW);

	led_blink_fast(LED_BLUE, 2);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, val,
				 sizeof(*val));
}

static ssize_t bt_set_distance_slow(struct bt_conn *conn,
				    const struct bt_gatt_attr *attr,
				    const void *buf, uint16_t len,
				    uint16_t offset, uint8_t flags)
{
	const uint32_t *msg;

	if (k_mutex_lock(&state_mtx, K_NO_WAIT)) {
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	if (state != SM_IDLE) {
		k_mutex_unlock(&state_mtx);
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	msg = (const uint32_t *)buf;
	config_param_write(CONFIG_PARAM_DISTANCE_SLOW, msg);
	k_mutex_unlock(&state_mtx);
	LOG_INF("distance slow = %d", *msg);
	led_blink_fast(LED_BLUE, 2);

	return 0;
}

static ssize_t bt_get_distance_brake(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     void *buf, uint16_t len, uint16_t offset)
{
	const uint32_t *val;

	val = (const uint32_t *)config_param_get(CONFIG_PARAM_DISTANCE_BRAKE);

	led_blink_fast(LED_BLUE, 2);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, val,
				 sizeof(*val));
}

static ssize_t bt_set_distance_brake(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     const void *buf, uint16_t len,
				     uint16_t offset, uint8_t flags)
{
	const uint32_t *msg;

	if (k_mutex_lock(&state_mtx, K_NO_WAIT)) {
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	if (state != SM_IDLE) {
		k_mutex_unlock(&state_mtx);
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	msg = (const uint32_t *)buf;
	config_param_write(CONFIG_PARAM_DISTANCE_BRAKE, msg);
	k_mutex_unlock(&state_mtx);
	LOG_INF("distance brake = %d", *msg);
	led_blink_fast(LED_BLUE, 2);

	return 0;
}

static ssize_t bt_get_start_power(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  void *buf, uint16_t len, uint16_t offset)
{
	const uint32_t *val;

	val = (const uint32_t *)config_param_get(CONFIG_PARAM_START_POWER);

	led_blink_fast(LED_BLUE, 2);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, val,
				 sizeof(*val));
}

static ssize_t bt_set_start_power(struct bt_conn *conn,
				  const struct bt_gatt_attr *attr,
				  const void *buf, uint16_t len,
				  uint16_t offset, uint8_t flags)
{
	const uint32_t *msg;

	if (k_mutex_lock(&state_mtx, K_NO_WAIT)) {
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	if (state != SM_IDLE) {
		k_mutex_unlock(&state_mtx);
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	// semafor
	msg = (const uint32_t *)buf;

	if (*msg > CONFIG_MOTOR_PWM_PERIOD_US) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	config_param_write(CONFIG_PARAM_START_POWER, msg);
	k_mutex_unlock(&state_mtx);
	LOG_INF("start power = %d", *msg);
	led_blink_fast(LED_BLUE, 2);

	return 0;
}

static ssize_t bt_get_pid_kp(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	const uint32_t *val;

	val = (const uint32_t *)config_param_get(CONFIG_PARAM_PID_KP);

	led_blink_fast(LED_BLUE, 2);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, val,
				 sizeof(*val));
}

static ssize_t bt_set_pid_kp(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     const void *buf, uint16_t len, uint16_t offset,
			     uint8_t flags)
{
	const uint32_t *msg;

	if (k_mutex_lock(&state_mtx, K_NO_WAIT)) {
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	if (state != SM_IDLE) {
		k_mutex_unlock(&state_mtx);
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	msg = (const uint32_t *)buf;
	config_param_write(CONFIG_PARAM_PID_KP, msg);
	k_mutex_unlock(&state_mtx);
	LOG_INF("pid kp = %d", *msg);
	led_blink_fast(LED_BLUE, 2);

	return 0;
}

static ssize_t bt_get_pid_ki(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	const uint32_t *val;

	val = (const uint32_t *)config_param_get(CONFIG_PARAM_PID_KI);

	led_blink_fast(LED_BLUE, 2);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, val,
				 sizeof(*val));
}

static ssize_t bt_set_pid_ki(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     const void *buf, uint16_t len, uint16_t offset,
			     uint8_t flags)
{
	const uint32_t *msg;

	if (k_mutex_lock(&state_mtx, K_NO_WAIT)) {
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	if (state != SM_IDLE) {
		k_mutex_unlock(&state_mtx);
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	// semafor
	msg = (const uint32_t *)buf;
	config_param_write(CONFIG_PARAM_PID_KI, msg);
	k_mutex_unlock(&state_mtx);
	LOG_INF("pid ki = %d", *msg);
	led_blink_fast(LED_BLUE, 2);

	return 0;
}

static ssize_t bt_set_bt_pswd(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset,
			      uint8_t flags)
{
	const uint32_t *msg;

	if (k_mutex_lock(&state_mtx, K_NO_WAIT)) {
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	if (state != SM_IDLE) {
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	msg = (const uint32_t *)buf;
	config_param_write(CONFIG_PARAM_BT_AUTH_PSWD, msg);
	k_mutex_unlock(&state_mtx);
	LOG_INF("bt pswd set");
	led_blink_fast(LED_BLUE, 2);

	return 0;
}

static ssize_t bt_set_default_config(struct bt_conn *conn,
				     const struct bt_gatt_attr *attr,
				     const void *buf, uint16_t len,
				     uint16_t offset, uint8_t flags)
{
	const uint32_t *msg;

	if (k_mutex_lock(&state_mtx, K_NO_WAIT)) {
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	if (state != SM_IDLE) {
		return BT_GATT_ERR(BT_ATT_ERR_PROCEDURE_IN_PROGRESS);
	}

	msg = (const uint32_t *)buf;
	if (*msg != 0) {
		config_make_default_settings();
		k_mutex_unlock(&state_mtx);
		LOG_INF("default settings set");
		led_blink_fast(LED_BLUE, 2);
	}

	return 0;
}

static struct bt_uuid_128 name_uuid = BT_UUID_INIT_128(
	0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

static struct bt_uuid_128 name_enc_uuid = BT_UUID_INIT_128(
	0xf1, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
	0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12);

#define CPF_FORMAT_UTF8 0x19

static const struct bt_gatt_cpf name_cpf = {
	.format = CPF_FORMAT_UTF8,
};

/* Vendor Primary Service Declaration */
BT_GATT_SERVICE_DEFINE(sm_service,
	/* Vendor Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&name_uuid),
	BT_GATT_CHARACTERISTIC(&name_enc_uuid.uuid,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ_AUTHEN,
			       bt_show_state, NULL, NULL),
	BT_GATT_CUD("SM State", BT_GATT_PERM_READ),
	BT_GATT_CPF(&name_cpf),
	BT_GATT_CHARACTERISTIC(&name_enc_uuid.uuid,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE_AUTHEN,
			       NULL, bt_start_lap, NULL),
	BT_GATT_CUD("BT Button", BT_GATT_PERM_READ),
	BT_GATT_CPF(&name_cpf),
	BT_GATT_CHARACTERISTIC(&name_enc_uuid.uuid,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ_AUTHEN,
			       bt_show_lap_length, NULL, NULL),
	BT_GATT_CUD("Lap length", BT_GATT_PERM_READ),
	BT_GATT_CPF(&name_cpf),
	BT_GATT_CHARACTERISTIC(&name_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ_AUTHEN |
			       BT_GATT_PERM_WRITE_AUTHEN,
			       bt_get_speed_max, bt_set_speed_max, NULL),
	BT_GATT_CUD("Speed fast", BT_GATT_PERM_READ),
	BT_GATT_CPF(&name_cpf),
	BT_GATT_CHARACTERISTIC(&name_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ_AUTHEN |
			       BT_GATT_PERM_WRITE_AUTHEN,
			       bt_get_speed_min, bt_set_speed_min, NULL),
	BT_GATT_CUD("Speed slow", BT_GATT_PERM_READ),
	BT_GATT_CPF(&name_cpf),
	BT_GATT_CHARACTERISTIC(&name_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ_AUTHEN |
			       BT_GATT_PERM_WRITE_AUTHEN,
			       bt_get_speed_brake, bt_set_speed_brake,
			       NULL),
	BT_GATT_CUD("Speed brake", BT_GATT_PERM_READ),
	BT_GATT_CPF(&name_cpf),
	BT_GATT_CHARACTERISTIC(&name_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ_AUTHEN |
			       BT_GATT_PERM_WRITE_AUTHEN,
			       bt_get_distance_brake, bt_set_distance_brake,
			       NULL),
	BT_GATT_CUD("Distance slow", BT_GATT_PERM_READ),
	BT_GATT_CPF(&name_cpf),
	BT_GATT_CHARACTERISTIC(&name_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ_AUTHEN |
			       BT_GATT_PERM_WRITE_AUTHEN,
			       bt_get_distance_slow, bt_set_distance_slow,
			       NULL),
	BT_GATT_CUD("Distance brake", BT_GATT_PERM_READ),
	BT_GATT_CPF(&name_cpf),
	BT_GATT_CHARACTERISTIC(&name_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ_AUTHEN |
			       BT_GATT_PERM_WRITE_AUTHEN,
			       bt_get_start_power, bt_set_start_power,
			       NULL),
	BT_GATT_CUD("Start Power", BT_GATT_PERM_READ),
	BT_GATT_CPF(&name_cpf),
	BT_GATT_CHARACTERISTIC(&name_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ_AUTHEN |
			       BT_GATT_PERM_WRITE_AUTHEN,
			       bt_get_pid_kp, bt_set_pid_kp,
			       NULL),
	BT_GATT_CUD("PID K_P", BT_GATT_PERM_READ),
	BT_GATT_CPF(&name_cpf),
	BT_GATT_CHARACTERISTIC(&name_enc_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ_AUTHEN |
			       BT_GATT_PERM_WRITE_AUTHEN,
			       bt_get_pid_ki, bt_set_pid_ki,
			       NULL),
	BT_GATT_CUD("PID K_I", BT_GATT_PERM_READ),
	BT_GATT_CPF(&name_cpf),
	BT_GATT_CHARACTERISTIC(&name_enc_uuid.uuid,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE_AUTHEN,
			       NULL, bt_set_bt_pswd,
			       NULL),
	BT_GATT_CUD("BT PSWD", BT_GATT_PERM_READ),
	BT_GATT_CPF(&name_cpf),
	BT_GATT_CHARACTERISTIC(&name_enc_uuid.uuid,
			       BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_WRITE_AUTHEN,
			       NULL, bt_set_default_config,
			       NULL),
	BT_GATT_CUD("Factory Settings", BT_GATT_PERM_READ),
	BT_GATT_CPF(&name_cpf),
);

