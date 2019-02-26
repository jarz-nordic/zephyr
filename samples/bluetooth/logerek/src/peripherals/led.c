/*
 * Copyright (c) 2018 Jakub Rzeszutko all rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Jakub Rzeszutko.
 * 4. Name of Jakub Rzeszutko may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JAKUB RZESZUTKO ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <zephyr.h>
#include <gpio.h>
#include <logging/log.h>

#include "led.h"

LOG_MODULE_REGISTER(app_led, LOG_LEVEL_DBG);

struct k_delayed_work led_init_timer;

struct led_device_info {
	struct device *dev;
	char *name;
	u32_t pin;
} leds[] = {
	{ .name = LED0_GPIO_CONTROLLER, .pin = LED0_GPIO_PIN, },
	{ .name = LED1_GPIO_CONTROLLER, .pin = LED1_GPIO_PIN, },
	{ .name = LED2_GPIO_CONTROLLER, .pin = LED2_GPIO_PIN, },
	{ .name = LED3_GPIO_CONTROLLER, .pin = LED3_GPIO_PIN, }
};

static void led_init_timeout(struct k_work *work)
{
	/* Disable all LEDs */
	for (u8_t i = 0; i < ARRAY_SIZE(leds); i++) {
		gpio_pin_write(leds[i].dev, leds[i].pin, 1);
	}
}

int led_init(void)
{
	for (u8_t i = 0; i < ARRAY_SIZE(leds); i++) {
		leds[i].dev = device_get_binding(leds[i].name);
		if (!leds[i].dev) {
			LOG_ERR("Failed to get %s device", leds[i].name);
			return -ENODEV;
		}

		gpio_pin_configure(leds[i].dev, leds[i].pin, GPIO_DIR_OUT);
		gpio_pin_write(leds[i].dev, leds[i].pin, 1);
	}

	k_delayed_work_init(&led_init_timer, led_init_timeout);
	return 0;
}

int led_set(enum led_idx idx, bool status)
{
	if (idx >= LED_MAX) {
		return -ENODEV;
	}

	u32_t value = status ? 0 : 1;

	gpio_pin_write(leds[idx].dev, leds[idx].pin, value);
	LOG_DBG("LED[%d] status changed to: %s", idx, value ? "on" : "off");

	return 0;
}

int led_set_time(enum led_idx idx, bool status, size_t time)
{
	int ret = led_set(idx, status);

	if (ret == 0) {
		k_delayed_work_submit(&led_init_timer, K_MSEC(time));
	}

	return ret;
}

