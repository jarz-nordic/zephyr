/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2020 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <zephyr.h>
#include <stats/stats.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/pwm.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>

#include <shell/shell.h>
#include "config.h"
#include "motor.h"

LOG_MODULE_REGISTER(motor);

/* The devicetree node identifier for the "hbridge-en" alias. */
#define HBRIDGE_ENABLE_NODE DT_ALIAS(hbridge_en)

#if DT_NODE_HAS_STATUS(HBRIDGE_ENABLE_NODE, okay)
#define HBRIDGE_EN_LABEL	DT_GPIO_LABEL(HBRIDGE_ENABLE_NODE, gpios)
#define HBRIDGE_EN_PIN		DT_GPIO_PIN(HBRIDGE_ENABLE_NODE, gpios)
#define HBRIDGE_EN_FLAGS	DT_GPIO_FLAGS(HBRIDGE_ENABLE_NODE, gpios)
#else
/* A build error here means your board isn't set up to control Hbridge. */
#error "Unsupported board: HBRIDGE_EN devicetree alias is not defined"
#endif

/* The devicetree node identifier for the "pwm-ch0" alias. */
#define PWM_CH0_NODE DT_ALIAS(pwm_ch0)

#if DT_NODE_HAS_STATUS(PWM_CH0_NODE, okay)
#define PWM0_LABEL       DT_PWMS_LABEL(PWM_CH0_NODE)
#define PWM0_CHANNEL     DT_PWMS_CHANNEL(PWM_CH0_NODE)
#define PWM0_FLAGS       DT_PWMS_FLAGS(PWM_CH0_NODE)
#else
/* A build error here means your board isn't set up to use motor with PWM. */
#error "Unsupported board: PWM_CH0 devicetree alias is not defined"
#endif

/* The devicetree node identifier for the "pwm-ch1" alias. */
#define PWM_CH1_NODE DT_ALIAS(pwm_ch1)

#if DT_NODE_HAS_STATUS(PWM_CH1_NODE, okay)
#define PWM1_LABEL       DT_PWMS_LABEL(PWM_CH1_NODE)
#define PWM1_CHANNEL     DT_PWMS_CHANNEL(PWM_CH1_NODE)
#define PWM1_FLAGS       DT_PWMS_FLAGS(PWM_CH1_NODE)
#else
/* A build error here means your board isn't set up to use motor with PWM. */
#error "Unsupported board: PWM_CH1 devicetree alias is not defined"
#endif

#define PWM_PERIOD_MS (100u)

struct motor_cb {
	enum motor_drv_direction current_dir;
};
static struct motor_cb motor_cb;

static void hbridge_enable(bool enable)
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(HBRIDGE_EN_LABEL);
	if (dev == NULL) {
		LOG_ERR("%s: Cannot bind HBRIDGE_EN", __FUNCTION__);
		return;
	}

	ret = gpio_pin_set(dev, HBRIDGE_EN_PIN, (int)enable);
	if (ret) {
		LOG_ERR("%s: Cannot set output", __FUNCTION__);
		return;
	}
}

static int hbridge_init(void)
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(HBRIDGE_EN_LABEL);
	if (dev == NULL) {
		LOG_ERR("Cannot bind HBRIDGE_EN");
		return -ENODEV;
	}
	ret = gpio_pin_configure(dev, HBRIDGE_EN_PIN,
				 GPIO_OUTPUT_ACTIVE | HBRIDGE_EN_FLAGS);
        if (ret < 0) {
                return -EINVAL;
        }

	hbridge_enable(false);

	return 0;
}

static int pwm_init(void)
{
	const struct device *dev;

	dev = device_get_binding(PWM0_LABEL);
        if (dev == NULL) {
                LOG_ERR("Cannot bind PWM0 device");
                return -ENODEV;
        }

	dev = device_get_binding(PWM1_LABEL);
        if (dev == NULL) {
                LOG_ERR("Cannot bind PWM1 device");
                return -ENODEV;
        }

	return 0;
}

static int qdec_init(void)
{
	return 0;
}

static int pwm_ch0_set(uint8_t duty_cycle)
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(PWM0_LABEL);
	if (dev == NULL) {
        	LOG_WRN("%s: Cannot bind PWM0 device", __FUNCTION__);
                return -ENODEV;
	}

	ret = pwm_pin_set_usec(dev, PWM0_CHANNEL, PWM_PERIOD_MS, duty_cycle,
				PWM0_FLAGS);
	if (ret) {
        	LOG_WRN("%s: error: [%d]", __FUNCTION__, ret);
	}

	return ret;

}

static int pwm_ch1_set(uint8_t duty_cycle)
{
	const struct device *dev;
	int ret;

	dev = device_get_binding(PWM1_LABEL);
	if (dev == NULL) {
        	LOG_WRN("%s: Cannot bind PWM1 device", __FUNCTION__);
                return -ENODEV;
	}

	ret = pwm_pin_set_usec(dev, PWM1_CHANNEL, PWM_PERIOD_MS, duty_cycle,
				PWM1_FLAGS);
	if (ret) {
        	LOG_WRN("%s: error: [%d]", __FUNCTION__, ret);
	}

	return ret;
}

static int pwms_set(enum motor_drv_direction direction, uint8_t duty_cycle)
{
	if (duty_cycle > 100) {
		LOG_WRN("%s: Requested Duty Cycle > 100", __FUNCTION__);
		duty_cycle = 100;
	}

	switch (direction)
	{
	case MOTOR_DRV_FORWARD:
		pwm_ch0_set(0);
		pwm_ch1_set(duty_cycle);
		break;
	case MOTOR_DRV_BACKWARD:
		pwm_ch0_set(duty_cycle);
		pwm_ch1_set(0);
		break;
	case MOTOR_DRV_BRAKE:
		pwm_ch0_set(100);
		pwm_ch1_set(100);
		break;
	case MOTOR_DRV_NEUTRAL:
		pwm_ch0_set(0);
		pwm_ch1_set(0);
		break;
	default:
		break;
	}

	return 0;
}

static uint8_t calculate_speed(uint32_t speed)
{
	return 20;
}

static int check_direction(enum motor_drv_direction direction)
{
	if (direction >= MOTOR_DRV_MAX) {
		LOG_WRN("Unrecognized motor request: %d", direction);
		return -EINVAL;
	}

	if (((direction == MOTOR_DRV_FORWARD) &&
	     (motor_cb.current_dir == MOTOR_DRV_BACKWARD)) ||
	    ((direction == MOTOR_DRV_BACKWARD) &&
	     (motor_cb.current_dir == MOTOR_DRV_FORWARD))) {
		LOG_WRN("Requested oposite direction. "
			"Current: %d, Requested: %d",
			motor_cb.current_dir, direction);

		return -EINVAL;
	}

	return 0;
}

int motor_init(void)
{
	int ret;

	ret = hbridge_init();
	if (ret) {
		LOG_ERR("Hbridge Init error: [%d]", ret);
		return ret;
	}

	ret = pwm_init();
	if (ret) {
		LOG_ERR("PWM Init error: [%d]", ret);
		return ret;
	}

	ret = qdec_init();
	if (ret) {
		LOG_ERR("QDEC Init retor: [%d]", ret);
		return ret;
	}

	motor_move(MOTOR_DRV_NEUTRAL, 0);
	hbridge_enable(false);

	return 0;
}

/* Function for driving the motor.
 *
 * @param[in] direction  movement direction
 * @param[in] speed	 impulses per second
 */
int motor_move(enum motor_drv_direction direction, uint32_t speed)
{
	uint8_t duty;
	int ret;

	ret = check_direction(direction);
	if (ret) {
		return ret;
	}

	hbridge_enable(true);
	duty = calculate_speed(speed);
	ret = pwms_set(direction, duty);

	return ret;
}
