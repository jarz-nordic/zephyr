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
#include <display/cfb.h>

#include "display.h"

static struct k_delayed_work epd_work;
static u8_t screen_id = SCREEN_MAIN;
static struct device *epd_dev;

static inline void display_refresh(void)
{
	k_delayed_work_submit(&epd_work, K_NO_WAIT);
}

static void show_statistics(void)
{
}

static void show_sensors_data(s32_t interval)
{

}

static void show_main(void)
{

}

static void epd_update(struct k_work *work)
{
	switch (screen_id) {
	case SCREEN_STATS:
		show_statistics();
		return;
	case SCREEN_SENSORS:
		show_sensors_data(K_SECONDS(2));
		return;
	case SCREEN_MAIN:
		show_main();
		return;
	}
}


int display_init(void)
{
	k_delayed_work_init(&epd_work, epd_update);
	return 0;
}

void display_screen_set(enum screen_ids id)
{

	display_refresh();
}

enum screen_ids display_screen_get(void)
{
	return screen_id;
}

void display_screen_increment(void)
{
	screen_id = (screen_id + 1) % SCREEN_LAST;

	display_refresh();
}
