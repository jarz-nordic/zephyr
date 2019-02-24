/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO driver sample
 *
 * This sample toggles LED1 and wait interrupt on BUTTON1.
 * Note that an internet pull-up is set on BUTTON1 as the button
 * only drives low when pressed.
 */
#include <zephyr.h>

#include <misc/printk.h>

#include <device.h>
#include <misc/util.h>


#include <hal/nrf_gpio.h>
#if defined(SW0_GPIO_CONTROLLER) && defined(LED0_GPIO_CONTROLLER)
#define GPIO_OUT_DRV_NAME LED0_GPIO_CONTROLLER
#define GPIO_OUT_PIN  LED0_GPIO_PIN
#define GPIO_IN_DRV_NAME SW0_GPIO_CONTROLLER
#define GPIO_INT_PIN  SW0_GPIO_PIN
#else
#error Change the pins based on your configuration. This sample \
	defaults to built-in buttons and LEDs
#endif

void main(void)
{

	nrf_gpio_cfg_output(32);
	nrf_gpio_pin_write(32, 0);

	return;

	while (1) {
		k_sleep(SECONDS(2));
	}
}
