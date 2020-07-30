/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#define POT_INCLUDE_COMPARE_FUNCTION

#include <pot.h>

static struct pot_element **elements;
static uint32_t	     arr_size;
static uint32_t	     write;

static void rebase_push(int i)
{
	if (i <= 1) {
		// do not need to rebase
		POT_LOG_DEBUG("No need to rebase\n");
		return;
	}

	uint8_t curr_elem = i - 1;
	uint8_t prev_elem = (i / 2) - 1;

	struct pot_element *p_prev_elem;

	if (pot_compare_elems(elements[curr_elem], elements[prev_elem])) {
		p_prev_elem = elements[prev_elem];
		elements[prev_elem] = elements[curr_elem];
		elements[curr_elem] = p_prev_elem;
		POT_LOG_DEBUG("Recurrence:\n");
		rebase_push(i / 2);
	} else {
		POT_LOG_DEBUG("No need to swap!\n");
	}
}

static void rebase_pop(int i)
{
	if (i > write) {
		// do not need to rebase
		POT_LOG_DEBUG("No need to rebase\n");
		return;
	}

	uint8_t curr_elem = i - 1;
	uint8_t next_elem1 = (i * 2) - 1;
	uint8_t next_elem2 = (i * 2);

	if (next_elem1 >= write) {
		// do not need to rebase
		POT_LOG_DEBUG("No need to rebase. Level below is empty.\n");
		return;
	}

	struct pot_element *p_curr_elem;

	if (pot_compare_elems(elements[next_elem1], elements[curr_elem])) {
		p_curr_elem = elements[curr_elem];
		elements[curr_elem] = elements[next_elem1];
		elements[next_elem1] = p_curr_elem;

		rebase_pop(i * 2);
	}

	if (next_elem2 >= write) {
		// do not need to rebase
		POT_LOG_DEBUG("No need to rebase. No last element.\n");
		return;
	}

	if (pot_compare_elems(elements[next_elem2], elements[curr_elem])) {
		p_curr_elem = elements[curr_elem];
		elements[curr_elem] = elements[next_elem2];
		elements[next_elem2] = p_curr_elem;

		rebase_pop(i * 2 + 1);

		if (pot_compare_elems(elements[next_elem1],
				      elements[curr_elem])) {
			p_curr_elem = elements[curr_elem];
			elements[curr_elem] = elements[next_elem1];
			elements[next_elem1] = p_curr_elem;

			rebase_pop(i * 2);
		}
	}
}

void pot_init(struct pot_element **p_array, uint32_t size)
{
	arr_size = size;
	elements = p_array;
	for (int i = 0; i < arr_size; ++i) {
		elements[i] = NULL;
	}
	write = 0;
}

int pot_push(struct pot_element *p_elem)
{
	// Check if we can add element
	if ((write >= arr_size) && (p_elem)) {
		return -ENOMEM;
	}

	// Add element
	elements[write] = p_elem;
	write++;

	// Rebase the tree
	rebase_push(write);
	return 0;
}

struct pot_element *pot_pop(void)
{
	struct pot_element *p_elem = elements[0];
	write--;

	elements[0] = elements[write];
	elements[write] = NULL;

	rebase_pop(1);

	return p_elem;
}

struct pot_element *pot_get(void)
{
	if (!elements) {
		return NULL;
	} else {
		return elements[0];
	}
}

int pot_remove(struct pot_element *p_elem)
{
	int i;
	int found = -1;

	for (i = 0; i < arr_size; ++i) {
		if (elements[i] == p_elem) {
			found = i;
			break;
		}
	}

	if (found >= 0) {
		write--;
		elements[i] = elements[write];
		elements[write] = NULL;
		rebase_pop(i + 1);
		return 0;
	}
	return -EAGAIN;
}
