/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <shell/shell.h>
#include <shell/shell_getopt.h>

void z_shell_getopt_init(struct getopt_s *vars)
{
	getoptvars_init(vars);
}

struct getopt_s *shell_getopt_state_get(const struct shell *shell)
{
	return &shell->ctx->getopt_vars;
}
