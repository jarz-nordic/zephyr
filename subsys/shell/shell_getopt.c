/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <shell/shell.h>
#include <shell/shell_getopt.h>

void z_shell_getopt_init(struct z_getopt_state *state)
{
	z_getopt_init(state);
}

int shell_getopt(const struct shell *shell, int argc, char *const argv[],
		 const char *options)
{
	if (!IS_ENABLED(CONFIG_SHELL_GETOPT)) {
		return -1;
	}

	__ASSERT_NO_MSG(shell);

	return z_getopt(&shell->ctx->getopt_state, argc, argv, options);
}

struct z_getopt_state *shell_getopt_state_get(const struct shell *shell)
{
	if (!IS_ENABLED(CONFIG_SHELL_GETOPT)) {
		return NULL;
	}

	__ASSERT_NO_MSG(shell);

	return &shell->ctx->getopt_state;
}
