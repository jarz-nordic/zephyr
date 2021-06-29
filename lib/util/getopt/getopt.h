/****************************************************************************
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * include/nuttx/lib/getopt.h
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

#ifndef __INCLUDE_NUTTX_LIB_GETOPT_H
#define __INCLUDE_NUTTX_LIB_GETOPT_H

#include <stdbool.h>
#include "getopt_unistd.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This structure encapsulates all variables associated with getopt().
 * These variables are thread specific (shell instance specific)
 */
struct getopt_s {
  /* Part of the implementation of the public getopt() interface */

  char	*go_optarg;	/* Optional argument following option */
  int	go_opterr;	/* Print error message */
  int	go_optind;	/* Index into argv */
  int	go_optopt;	/* unrecognized option character */

  /* Internal getopt() state */
  char	*go_optptr;		/* Current parsing location */
  bool	go_binitialized;	/* true:  getopt() has been initialized */
};

struct option {
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

/* Refering below values is safe only if only one thread / shell instance is
 * using getopt, otherwise each getopt call by the different thread will be
 * overwriting them.
 * Each shell instance has its own getopt_s structure so the function will
 * always work correctly. However, below global variables may not be correct.
 * If there are multiple instances of the shell and each uses getopt then
 * the current state of the function can be obtained by calling
 * getopt_state_get();
 */
extern char *optarg;
extern int opterr;
extern int optind;
extern int optopt;

/*
 * Name: getopt_state_get(void)
 *
 * Description:
 *
 *	Function returns getopt state for currently executed thread.
 *	If only one thread is accessing getopt functions user can safely
 *	use global variables: optarg, opterr, optind, optopt.
 */
struct getopt_s *getopt_state_get(void);

/*
 * Name: getopt
 *
 * Description:
 *   getopt() parses command-line arguments.  Its arguments argc and argv
 *   are the argument count and array as passed to the main() function on
 *   program invocation.  An element of argv that starts with '-' is an
 *   option element. The characters of this element (aside from the initial
 *   '-') are option characters. If getopt() is called repeatedly, it
 *   returns successively each of the option characters from each of the
 *   option elements.
 *
 *   If getopt() finds another option character, it returns that character,
 *   updating the external variable optind and a static variable nextchar so
 *   that the next call to getopt() can resume the scan with the following
 *   option character or argv-element.
 *
 *   If there are no more option characters, getopt() returns -1. Then optind
 *   is the index in argv of the first argv-element that is not an option.
 *
 *   The 'optstring' argument is a string containing the legitimate option
 *   characters. If such a character is followed by a colon, this indicates
 *   that the option requires an argument.  If an argument is required for an
 *   option so getopt() places a pointer to the following text in the same
 *   argv-element, or the text of the following argv-element, in optarg.
 *
 *   NOTES:
 *   1. opterr is not supported and this implementation of getopt() never
 *      printfs error messages.
 *   2. getopt is NOT threadsafe!
 *   3. This version of getopt() does not reset global variables until
 *      -1 is returned.  As a result, your command line parsing loops
 *      must call getopt() repeatedly and continue to parse if other
 *      errors are returned ('?' or ':') until getopt() finally returns -1.
 *      (You can also set optind to -1 to force a reset).
 *   4. Standard getopt() permutes the contents of argv as it scans, so that
 *      eventually all the nonoptions are at the end.  This implementation
 *      does not do this.
 *
 * Returned Value:
 *   If an option was successfully found, then getopt() returns the option
 *   character. If all command-line options have been parsed, then getopt()
 *   returns -1.  If getopt() encounters an option character that was not
 *   in optstring, then '?' is returned. If getopt() encounters an option
 *   with a missing argument, then the return value depends on the first
 *   character in optstring: if it is ':', then ':' is returned; otherwise
 *   '?' is returned.
 *
 */
int getopt(int argc, char * const argv[], const char *optstring);

/*
 * Name: getopt_long
 *
 * Description:
 *	The getopt_long() function works like getopt() except that it also
 *	accepts long options, started with two dashes. (If the program accepts
 *	only long options, then optstring should be specified as an empty
 *	string (""), not NULL.) Long option names may be abbreviated if the
 *	abbreviation is unique or is an exact match for some defined option
 *
 */
int getopt_long(int argc, char * const argv[],
		const char *optstring,
		const struct option *longopts,
		int *longindex);

/* Name: getopt_long_only
 *
 * Description:
 *   getopt_long_only() is like getopt_long(), but '-' as well as "--" can
 *   indicate a long option. If an option that starts with '-' (not "--")
 *   doesn't match a long option, but does match a short option, it is
 *   parsed as a short option instead.
 *
 */
int getopt_long_only(int argc, char * const argv[],
		     const char *optstring,
		     const struct option *longopts,
		     int *longindex);

#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_NUTTX_LIB_GETOPT_H */
