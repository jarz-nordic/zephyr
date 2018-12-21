/*
 * Copyright (c) 2018
 *	Jakub Rzeszutko all rights reserved.
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

#include "button.h"

static int configure_button(void)
{
	static struct gpio_callback button_cb;

	gpio = device_get_binding(SW0_GPIO_CONTROLLER);
	if (!gpio) {
		return -ENODEV;
	}

	gpio_pin_configure(gpio, SW0_GPIO_PIN,
			   (GPIO_DIR_IN | GPIO_INT |  PULL_UP | EDGE));

	gpio_init_callback(&button_cb, button_interrupt, BIT(SW0_GPIO_PIN));
	gpio_add_callback(gpio, &button_cb);

	gpio_pin_enable_callback(gpio, SW0_GPIO_PIN);

	return 0;
}
int button_init(void)
{

	return 0;
}

int button_get(enum button_idx idx, bool *status)
{
	/* reel board implementation */
	if (idx > BUTTON1) {
		return -ENODEV;
	}

	return 0;
}
