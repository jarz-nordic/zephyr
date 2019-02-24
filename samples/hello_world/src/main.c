/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <hal/nrf_gpio.h>

extern void __uarte_unint(void);

void main(void)
{
	volatile NRF_GPIO_Type *gpio = NRF_P0;
	printk("Hello World! %s\n", CONFIG_BOARD);
	nrf_gpio_cfg_output(32);
	nrf_gpio_pin_write(32, 0);
	__uarte_unint();

	gpio->PIN_CNF[DT_NORDIC_NRF_UARTE_0_RX_PIN] =
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
		(GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos) |
		(GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos);

	return;
	nrf_gpio_cfg_output(13);
	nrf_gpio_cfg_output(15);
	nrf_gpio_cfg_output(16);
	nrf_gpio_cfg_output(17);
	nrf_gpio_cfg_output(22);
	nrf_gpio_cfg_output(23);
	nrf_gpio_cfg_output(24);
	nrf_gpio_cfg_output(25);
	nrf_gpio_cfg_output(26);
	nrf_gpio_cfg_output(27);

	nrf_gpio_pin_write(26, 0);
	nrf_gpio_pin_write(27, 0);
	nrf_gpio_pin_write(23, 0);
	nrf_gpio_pin_write(22, 0);
	nrf_gpio_pin_write(24, 0);
	nrf_gpio_pin_write(25, 0);

	nrf_gpio_pin_write(13, 0);
	nrf_gpio_pin_write(15, 0);
	nrf_gpio_pin_write(16, 0);
	nrf_gpio_pin_write(17, 0);
	nrf_gpio_pin_write(32, 0);
}
