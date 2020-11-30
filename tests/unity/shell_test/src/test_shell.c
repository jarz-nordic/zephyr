/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <unity.h>
#include <stdbool.h>
#include <shell/shell.h>

void setUp(void)
{
}

void test_uut_init(void)
{
}

void test_shell_prompt_change(void)
{
	int ret;
	struct shell shell;

	ret = shell_prompt_change(&shell, NULL);

	TEST_ASSERT_EQUAL(ret, -EINVAL);
}

/* It is required to be added to each test. That is because unity is using
 * different main signature (returns int) and zephyr expects main which does
 * not return value.
 */
extern int unity_main(void);

void main(void)
{
	(void)unity_main();
}
