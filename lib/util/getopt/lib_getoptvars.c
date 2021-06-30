/****************************************************************************
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * libs/libc/unistd/lib_getoptvars.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#include <zephyr.h>
#include <shell/shell.h>
#include "getopt_unistd.h"
#include <stdio.h>

/* Data is naturally process-specific in the KERNEL build so no special
 * access to process-specific global data is needed.
 */

static struct getopt_s g_getopt_vars =
{
	NULL,
	0,
	1,
	'?',
	NULL,
	false
};

/*
 * Name: getoptvars
 *
 * Description:
 *   Returns a pointer to to the thread-specific getopt() data.
 *
 */

struct getopt_s *getoptvars(void)
{/*
	if (IS_ENABLED(CONFIG_SHELL_GETOPT)) {
		k_tid_t tid = k_current_get();
		Z_STRUCT_SECTION_FOREACH(shell, sh) {
			if (tid == sh->ctx->tid) {
				return &sh->ctx->getopt_vars;
			}
		}
	}
*/
	/* If not a shell thread return a common pointer */
	return &g_getopt_vars;
}

void getoptvars_init(struct getopt_s *getopt_vars)
{
	optarg = NULL;
	opterr = 0;
	optind = 1;
	optopt = '?';

	if (getopt_vars == NULL) {
		g_getopt_vars.go_optarg = NULL;
		g_getopt_vars.go_opterr = 0;
		g_getopt_vars.go_optind = 1;
		g_getopt_vars.go_optopt = '?';
		g_getopt_vars.go_optptr = NULL;
		g_getopt_vars.go_binitialized = false;
		return;
	}

	getopt_vars->go_optarg = NULL;
	getopt_vars->go_opterr = 0;
	getopt_vars->go_optind = 1;
	getopt_vars->go_optopt = '?';
	getopt_vars->go_optptr = NULL;
	getopt_vars->go_binitialized = false;
}

struct getopt_s *getopt_state_get(void)
{
	return getoptvars();
}

void global_getopt_vars_set(struct getopt_s *go)
{
	if (go) {
		optarg = go->go_optarg;
		opterr = go->go_opterr;
		optind = go->go_optind;
		optopt = go->go_optopt;
	} else {
		optarg = g_getopt_vars.go_optarg;
		opterr = g_getopt_vars.go_opterr;
		optind = g_getopt_vars.go_optind;
		optopt = g_getopt_vars.go_optopt;
	}
}

