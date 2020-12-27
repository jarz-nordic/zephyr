/*
 * Copyright (c) 2020 Jakub Rzeszutko
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/types.h>

/*
 * This module is a simple LED control manager that controls one RGB diode.
 *
 * Assumptions of the module:
 *	1) Only 1 LED can be lit at a time.
 *	2) You can put 3 LED actions into the queue.
 *	3) If the LED is on continuously and there is an event in the queue,
 *	   the LED is switched off and the next event is supported.
 *	4) If the LED was blinking and the next event came, it will be locked
 *	   and only started when the LED has finished blinking the set number
 *	   of times.
 *	5) If the LED blinks continuously and a new event occurs, the blinking
 *	   is interrupted and the next event is supported.
 *
*/

/* Enum with available LED colors */
enum led_color {
	LED_GREEN,
	LED_RED,
	LED_BLUE,
/* -------- Internal use only ----*/
	LED_MAX,
};

/* Function starting LED thread
 *
 * @return  0 on sunccess
 */
int led_thread_init(void);

/* Function setting LED on and off *
 *
 * @param[in] color	led color from enum led_color
 * @param[in] enable	led state that will be set
 *
 * @retval 0		If successfully set
 * @retval -ENOMEM	No space in events fifo, try again later.
 */
int led_set(enum led_color color, bool enable);

/* Function for blinking with 100ms period.
 *
 * @param[in] color	led color
 * @param[in] cnt	number of blinks: (1..10) or 0xFF for continous blinking
 *
 * @retval 0		If successfully set.
 * @retval -EINVAL	Invalid input parameter.
 * @retval -ENOMEM	No space in events fifo, try again later.
 */
int led_blink_fast(enum led_color color, uint8_t cnt);

/* Function for blinking with 500ms period.
 *
 * @param[in] color	led color
 * @param[in] cnt	number of blinks: (1..254) or 255 for continous blinking
 *
 * @retval 0		If successfully set.
 * @retval -EINVAL	Invalid input parameter.
 * @retval -ENOMEM	No space in events fifo, try again later.
 */
int led_blink_slow(enum led_color color, uint8_t cnt);
