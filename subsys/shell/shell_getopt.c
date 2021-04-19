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

#ifdef CONFIG_SHELL_GETOPT_LONG
int shell_getopt_long(const struct shell *shell, int argc, char *const argv[],
		      const char *options, const struct z_option *long_options,
		      int *long_idx)
{
	__ASSERT_NO_MSG(shell);

	return z_getopt_long(&shell->ctx->getopt_state, argc, argv, options,
			     long_options, long_idx);
}

int shell_getopt_long_only(const struct shell *shell, int argc,
			   char *const argv[], const char *options,
			   const struct z_option *long_options, int *long_idx)
{
	__ASSERT_NO_MSG(shell);

	return z_getopt_long_only(&shell->ctx->getopt_state, argc, argv, options,
				  long_options, long_idx);

}

#endif /* CONFIG_SHELL_GETOPT_LONG */
