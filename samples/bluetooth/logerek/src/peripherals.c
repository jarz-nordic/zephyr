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
#include <logging/log.h>

#include "peripherals/led.h"
#include "peripherals/button.h"
#include "peripherals/sensory.h"
#include "peripherals/display.h"

LOG_MODULE_REGISTER(app_peripherals, LOG_LEVEL_DBG);

int peripherals_init(void)
{
	int err = 0;

//	err = led_init();
	if (err) {
		LOG_ERR("LED initialization failed: err %d", err);
		return err;
	}

//	err = button_init();
	if (err) {
		LOG_ERR("Button initialization failed: err %d", err);
		return err;
	}

//	err = display_init();
	if (err) {
		LOG_ERR("Display initialization failed: err %d", err);
		return err;
	}

	/* starting sensors thread */
	err = sensory_init();
	if (err) {
		LOG_ERR("Sensors initialization failed: err %d", err);
		return err;
	}

	LOG_INF("Peripherals initialized");

	return 0;
}
