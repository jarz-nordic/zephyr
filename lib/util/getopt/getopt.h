/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _GETOPT_H__
#define _GETOPT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr.h>

struct z_getopt_state {
	int opterr;	/* if error message should be printed */
	int optind;	/* index into parent argv vector */
	int optopt;	/* character checked for validity */
	int optreset;	/* reset getopt */
	char *optarg;	/* argument associated with option */

	char *place;	/* option letter processing */

#if CONFIG_GETOPT_LONG
	int nonopt_start;
	int nonopt_end;
#endif
};

/* Function intializes getopt_state structure */
void z_getopt_init(struct z_getopt_state *state);

/*
 * getopt --
 *	Parse argc/argv argument vector.
 */
int z_getopt(struct z_getopt_state *const state, int nargc,
	     char *const nargv[], const char *ostr);

#define no_argument        0
#define required_argument  1
#define optional_argument  2

struct z_option {
	/* name of long option */
	const char *name;
	/*
	 * one of no_argument, required_argument, and optional_argument:
	 * whether option takes an argument
	 */
	int has_arg;
	/* if not NULL, set *flag to val when option found */
	int *flag;
	/* if flag not NULL, value to set *flag to; else return value */
	int val;
};

/*
 * z_getopt_long --
 *	Parse argc/argv argument vector.
 */
int
z_getopt_long(struct z_getopt_state *state, int nargc, char *const *nargv,
	      const char *options, const struct z_option *long_options,
	      int *idx);

/*
 * z_getopt_long_only --
 *	Parse argc/argv argument vector.
 */
int
z_getopt_long_only(struct z_getopt_state *state, int nargc, char *const *nargv,
		   const char *options, const struct z_option *long_options,
		   int *idx);

#ifdef __cplusplus
}
#endif

#endif /* _GETOPT_H__ */
