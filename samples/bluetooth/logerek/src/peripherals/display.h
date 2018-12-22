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

#ifndef DISPLAY_H__
#define DISPLAY_H__

enum screen_ids {
	SCREEN_BOOT = 0,
	SCREEN_SENSORS = 1,
	SCREEN_PYSZCZEK = 2,
	SCREEN_LAST,
};

enum font_size {
	FONT_BIG = 0,
	FONT_MEDIUM = 1,
	FONT_SMALL = 2,
};

/* Function initializing display */
void board_show_text(enum font_size font, const char *text, bool center,
		     s32_t duration);
int display_init(void);
int display_screen(enum screen_ids id);
enum screen_ids display_screen_get(void);
void display_screen_increment(void);

#define display_small(text, center, time)	\
	board_show_text(FONT_SMALL, text, center, time)

#define display_medium(text, center, time)	\
	board_show_text(FONT_MEDIUM, text, center, time)

#define display_big(text, center, time)		\
	board_show_text(FONT_BIG, text, center, time)

#endif /* DISPLAY_H__ */
