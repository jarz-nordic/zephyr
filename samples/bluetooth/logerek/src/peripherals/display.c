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
#include <sensor.h>
#include <version.h>
#include <logging/log.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "sensory.h"
#include "display.h"

LOG_MODULE_REGISTER(app_display, LOG_LEVEL_INF);

static struct k_delayed_work epd_work;
static u8_t screen_id = SCREEN_BOOT;
static struct device *epd_dev;
static char str_buf[280];

struct font_info {
	u8_t columns;
} fonts[] = {
	[FONT_BIG] =    { .columns = 12 },
	[FONT_MEDIUM] = { .columns = 16 },
	[FONT_SMALL] =  { .columns = 25 },
};


static size_t get_len(enum font_size font, const char *text)
{
	const char *space = NULL;
	s32_t i;

	for (i = 0; i <= fonts[font].columns; i++) {
		switch (text[i]) {
		case '\n':
		case '\0':
			return i;
		case ' ':
			space = &text[i];
			break;
		default:
			continue;
		}
	}

	/* If we got more characters than fits a line, and a space was
	 * encountered, fall back to the last space.
	 */
	if (space) {
		return space - text;
	}

	return fonts[font].columns;
}

static size_t print_line(enum font_size font_size, int row, const char *text,
			 size_t len, bool center)
{
	u8_t font_height, font_width;
	u8_t line[fonts[FONT_SMALL].columns + 1];
	int pad;

	cfb_framebuffer_set_font(epd_dev, font_size);

	len = MIN(len, fonts[font_size].columns);
	memcpy(line, text, len);
	line[len] = '\0';

	if (center) {
		pad = (fonts[font_size].columns - len) / 2;
	} else {
		pad = 0;
	}

	cfb_get_font_size(epd_dev, font_size, &font_width, &font_height);

	if (cfb_print(epd_dev, line, font_width * pad, font_height * row)) {
		LOG_ERR("Failed to print a string");
	}

	return len;
}

void board_show_text(enum font_size font, const char *text, bool center,
		     s32_t duration)
{
	int i;

	cfb_framebuffer_clear(epd_dev, false);

	for (i = 0; i < 3; i++) {
		size_t len;

		while (*text == ' ' || *text == '\n') {
			text++;
		}

		len = get_len(font, text);
		if (!len) {
			break;
		}

		text += print_line(font, i, text, len, center);
		if (!*text) {
			break;
		}
	}

	cfb_framebuffer_finalize(epd_dev);

	if (duration != K_FOREVER) {
		k_delayed_work_submit(&epd_work, duration);
	}
}

static inline void display_refresh(void)
{
	k_delayed_work_submit(&epd_work, K_NO_WAIT);
}

static void show_boot_banner(int time)
{
	snprintf(str_buf, sizeof(str_buf), "Booting Zephyr OS\n"
		"Version: %s",
		 KERNEL_VERSION_STRING);

	display_medium(str_buf, false, 2000);
}

static void show_sensors_data(s32_t interval)
{
	static s32_t old_external_tmp = INVALID_SENSOR_VALUE;
	static s32_t old_tmp = INVALID_SENSOR_VALUE;
	static s32_t old_hum = INVALID_SENSOR_VALUE;

	s32_t external_tmp = sensory_get_temperature_external();
	s32_t temperature = sensory_get_temperature();
	s32_t humidity = sensory_get_humidity();

	u16_t len = 0U;
	u16_t len2 = 0U;

	if ((temperature == old_tmp) && (humidity == old_hum) &&
	    (old_external_tmp != external_tmp)) {
		return;
	}

	old_external_tmp = external_tmp;
	old_tmp = temperature;
	old_hum = humidity;

	if (temperature != INVALID_SENSOR_VALUE) {
		len = snprintf(str_buf, sizeof(str_buf),
				"Dom : %d.%dC\n",
				temperature / 10, abs(temperature % 10));
	} else {
		len = snprintf(str_buf, sizeof(str_buf),
				"Dom : N/A C\n");
	}

	if (humidity != INVALID_SENSOR_VALUE) {
		len2 = snprintf(str_buf + len, sizeof(str_buf) - len,
				"Dom : %3d%% H\n",
				humidity);
	} else {
		len2 = snprintf(str_buf + len, sizeof(str_buf) - len,
				"Dom : N/A %% H\n");
	}

	if (old_external_tmp != INVALID_SENSOR_VALUE) {
		len = snprintf(str_buf + len + len2,
				sizeof(str_buf) - len - len2,
				"Pole:%c%d.%dC\n",
				external_tmp > 0 ? ' ' : '-',
				abs(external_tmp / 10),
				abs(external_tmp % 10));
	} else {
		len = snprintf(str_buf + len + len2,
				sizeof(str_buf) - len - len2,
				"Pole: N/A C");
	}

	display_big(str_buf, false, interval);
}

static void show_main(void)
{

}

static void epd_update(struct k_work *work)
{
	switch (screen_id) {
	case SCREEN_BOOT:
		show_boot_banner(K_SECONDS(5));
		break;
	case SCREEN_SENSORS:
		show_sensors_data(K_FOREVER);
		break;
	case SCREEN_PYSZCZEK:
		show_main();
		break;
	default:
		LOG_ERR("fatal display error");
		return;
	}

	screen_id = SCREEN_SENSORS;
}


int display_init(void)
{
	epd_dev = device_get_binding(DT_SOLOMON_SSD1673FB_0_LABEL);
	if (epd_dev == NULL) {
		LOG_ERR("SSD1673 device not found");
		return -ENODEV;
	}

	if (cfb_framebuffer_init(epd_dev)) {
		LOG_ERR("Framebuffer initialization failed");
		return -EIO;
	}

	k_delayed_work_init(&epd_work, epd_update);

	cfb_framebuffer_clear(epd_dev, true);

	k_delayed_work_submit(&epd_work, K_NO_WAIT);

	return 0;
}

int display_screen(enum screen_ids id)
{
	if (id >= SCREEN_LAST) {
		LOG_ERR("Requested screem does not exist");
		return -EINVAL;
	}

	screen_id = id;
	display_refresh();

	return 0;
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
