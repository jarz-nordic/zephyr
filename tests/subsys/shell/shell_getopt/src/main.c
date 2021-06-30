/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Interactive shell test suite
 *
 */

#include <zephyr.h>
#include <ztest.h>
#include <string.h>

#include <shell/shell_dummy.h>
#include <shell/shell_getopt.h>

static void test_getopt_basic(void)
{
	static const char *const nargv[] = {
		"cmd_name",
		"-b",
		"-a",
		"-h",
		"-c",
		"-l",
		"-h",
		"-a",
		"-i",
		"-w",
	};
	static const char *accepted_opt = "abchw";
	static const char *expected = "bahc?ha?w";
	size_t argc = ARRAY_SIZE(nargv);
	int cnt = 0;
	int c;
	char **argv;

	argv = (char **)nargv;

	getoptvars_init(NULL);

	do {
		c = getopt(argc, argv, accepted_opt);
		if (cnt >= strlen(expected)) {
			break;
		}

		zassert_equal(c, expected[cnt++], "unexpected opt character");
	} while (c != -1);
}

enum getopt_idx {
	GETOPT_IDX_CMD_NAME,
	GETOPT_IDX_OPTION1,
	GETOPT_IDX_OPTION2,
	GETOPT_IDX_OPTARG
};

static void test_getopt(void)
{
	static const char *test_opts = "ac:";
	static const char *const nargv[] = {
		[GETOPT_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_IDX_OPTION1] = "-a",
		[GETOPT_IDX_OPTION2] = "-c",
		[GETOPT_IDX_OPTARG] = "foo",
	};
	int argc = ARRAY_SIZE(nargv);
	char **argv;
	int c;

	argv = (char **)nargv;

	getoptvars_init(NULL);

	/* Test uknown option */
	c = getopt(argc, argv, test_opts);

	zassert_equal(c, 'a', "unexpected opt character");
	c = getopt(argc, argv, test_opts);
	zassert_equal(c, 'c', "unexpected opt character");
	c = getopt(argc, argv, test_opts);
	zassert_equal(0, strcmp(argv[GETOPT_IDX_OPTARG], optarg),
		      "unexpected optarg result");
}

enum getopt_long_idx {
	GETOPT_LONG_IDX_CMD_NAME,
	GETOPT_LONG_IDX_VERBOSE,
	GETOPT_LONG_IDX_OPT,
	GETOPT_LONG_IDX_OPTARG
};

static void test_getopt_long(void)
{
	/* Below test is based on example
	 * https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
	 */
	int verbose_flag = 0;
	/* getopt_long stores the option index here. */
	int option_index = 0;
	char **argv;
	int c;
	struct option long_options[] = {
		/* These options set a flag. */
		{"verbose",	no_argument,		&verbose_flag, 1},
		{"brief",	no_argument,		&verbose_flag, 0},
		/* These options don’t set a flag.
		 * We distinguish them by their indices.
		 */
		{"add",		no_argument,		0, 'a'},
		{"create",	required_argument,	0, 'c'},
		{"delete",	required_argument,	0, 'd'},
		{"long",	required_argument,	0, 'e'},
		{0, 0, 0, 0}
	};
	static const char *accepted_opt = "ac:d:e:";

	static const char *const argv1[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--verbose",
		[GETOPT_LONG_IDX_OPT] = "--create",
		[GETOPT_LONG_IDX_OPTARG] = "some_file",
	};
	int argc1 = ARRAY_SIZE(argv1);

	static const char *const argv2[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		[GETOPT_LONG_IDX_OPT] = "-d",
		[GETOPT_LONG_IDX_OPTARG] = "other_file",
	};
	int argc2 = ARRAY_SIZE(argv2);

	static const char *const argv3[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		[GETOPT_LONG_IDX_OPT] = "-a",
	};
	int argc3 = ARRAY_SIZE(argv3);

	/* this test distinguish getopt_long and getopt_long_only functions */
	static const char *const argv4[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		/* below should not be interpreted as "--long/-e" option */
		[GETOPT_LONG_IDX_OPT] = "-l",
		[GETOPT_LONG_IDX_OPTARG] = "long_argument",
	};
	int argc4 = ARRAY_SIZE(argv4);

	/* Test scenario 1 */
	argv = (char **)argv1;
	getoptvars_init(NULL);
	c = getopt_long(argc1, argv, accepted_opt,
			long_options, &option_index);
	printf("verbose flag = %d\n", verbose_flag);
	zassert_equal(verbose_flag, 1, "verbose flag expected");
	return;
	c = getopt_long(argc1, argv, accepted_opt,
			long_options, &option_index);
	return;
	zassert_equal('c', c, "unexpected option");

	zassert_equal(0, strcmp(optarg, argv[GETOPT_LONG_IDX_OPTARG]),
		      "unexpected optarg");
	c = getopt_long(argc1, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal(-1, c, "shell_getopt_long shall return -1");

	return;
	/* Test scenario 2 */
	argv = (char **)argv2;
	getoptvars_init(NULL);
	c = getopt_long(argc2, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = getopt_long(argc2, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal('d', c, "unexpected option");
	zassert_equal(0, strcmp(optarg, argv[GETOPT_LONG_IDX_OPTARG]),
		      "unexpected optarg");
	c = getopt_long(argc2, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal(-1, c, "shell_getopt_long shall return -1");

	/* Test scenario 3 */
	argv = (char **)argv3;
	getoptvars_init(NULL);
	c = getopt_long(argc3, argv, accepted_opt,
			      long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = getopt_long(argc3, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal('a', c, "unexpected option");
	c = getopt_long(argc3, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal(-1, c, "shell_getopt_long shall return -1");

	/* Test scenario 4 */
	argv = (char **)argv4;
	getoptvars_init(NULL);
	c = getopt_long(argc4, argv, accepted_opt,
			long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = getopt_long(argc4, argv, accepted_opt,
			long_options, &option_index);

	/* Function was called with option '-l'. It is expected it will be
	 * NOT evaluated to '--long' which has flag 'e'.
	 */
	zassert_not_equal('e', c, "unexpected option match");
	c = getopt_long(argc4, argv, accepted_opt,
			long_options, &option_index);
}

static void test_getopt_long_only(void)
{
	/* Below test is based on example
	 * https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
	 */
	int verbose_flag = 0;
	/* getopt_long stores the option index here. */
	int option_index = 0;
	char **argv;
	int c;
	struct option long_options[] = {
		/* These options set a flag. */
		{"verbose",	no_argument,		&verbose_flag, 1},
		{"brief",	no_argument,		&verbose_flag, 0},
		/* These options don’t set a flag.
		 * We distinguish them by their indices.
		 */
		{"add",		no_argument,		0, 'a'},
		{"create",	required_argument,	0, 'c'},
		{"delete",	required_argument,	0, 'd'},
		{"long",	required_argument,	0, 'e'},
		{0, 0, 0, 0}
	};
	static const char *accepted_opt = "ac:d:e:";

	static const char *const argv1[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--verbose",
		[GETOPT_LONG_IDX_OPT] = "--create",
		[GETOPT_LONG_IDX_OPTARG] = "some_file",
	};
	int argc1 = ARRAY_SIZE(argv1);

	static const char *const argv2[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		[GETOPT_LONG_IDX_OPT] = "-d",
		[GETOPT_LONG_IDX_OPTARG] = "other_file",
	};
	int argc2 = ARRAY_SIZE(argv2);

	static const char *const argv3[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		[GETOPT_LONG_IDX_OPT] = "-a",
	};
	int argc3 = ARRAY_SIZE(argv3);

	/* this test distinguish getopt_long and getopt_long_only functions */
	static const char *const argv4[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		/* below should be interpreted as "--long/-e" option */
		[GETOPT_LONG_IDX_OPT] = "-l",
		[GETOPT_LONG_IDX_OPTARG] = "long_argument",
	};
	int argc4 = ARRAY_SIZE(argv4);

	/* Test scenario 1 */
	argv = (char **)argv1;
	getoptvars_init(NULL);
	c = getopt_long_only(argc1, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal(verbose_flag, 1, "verbose flag expected");
	c = getopt_long_only(argc1, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal('c', c, "unexpected option");
	zassert_equal(0, strcmp(optarg, argv[GETOPT_LONG_IDX_OPTARG]),
		      "unexpected optarg");
	c = getopt_long_only(argc1, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal(-1, c, "shell_getopt_long_only shall return -1");

	/* Test scenario 2 */
	argv = (char **)argv2;
	getoptvars_init(NULL);
	c = getopt_long_only(argc2, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = getopt_long_only(argc2, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal('d', c, "unexpected option");
	zassert_equal(0, strcmp(optarg, argv[GETOPT_LONG_IDX_OPTARG]),
		      "unexpected optarg");
	c = getopt_long_only(argc2, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal(-1, c, "shell_getopt_long_only shall return -1");

	/* Test scenario 3 */
	argv = (char **)argv3;
	getoptvars_init(NULL);
	c = getopt_long_only(argc3, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = getopt_long_only(argc3, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal('a', c, "unexpected option");
	c = getopt_long_only(argc3, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal(-1, c, "shell_getopt_long_only shall return -1");

	/* Test scenario 4 */
	argv = (char **)argv4;
	getoptvars_init(NULL);
	c = getopt_long_only(argc4, argv, accepted_opt,
			     long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = getopt_long_only(argc4, argv, accepted_opt,
			     long_options, &option_index);

	/* Function was called with option '-l'. It is expected it will be
	 * evaluated to '--long' which has flag 'e'.
	 */
	zassert_equal('e', c, "unexpected option");
	c = getopt_long_only(argc4, argv, accepted_opt,
			     long_options, &option_index);
}

void test_main(void)
{
	ztest_test_suite(shell_test_suite,
			ztest_unit_test(test_getopt_basic),
			ztest_unit_test(test_getopt),
			ztest_unit_test(test_getopt_long),
			ztest_unit_test(test_getopt_long_only)
			);

	ztest_run_test_suite(shell_test_suite);
}
