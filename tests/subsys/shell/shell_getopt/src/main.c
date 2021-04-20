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
	const struct shell *shell = shell_backend_dummy_get_ptr();
	char *argv[] = {
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
	const char *accepted_opt = "abchw";
	const char *expected = "bahc?ha?w";
	size_t argc = ARRAY_SIZE(argv);
	int c;
	int cnt = 0;

	z_shell_getopt_init(&shell->ctx->getopt_state);
	do {
		c = shell_getopt(shell, argc, argv, accepted_opt);
		if (cnt >= strlen(expected)) {
			break;
		}

		zassert_equal(c, expected[cnt++], "unexpected opt character");
	} while (c != -1);

	c = shell_getopt(shell, argc, argv, accepted_opt);
	zassert_equal(c, -1, "unexpected opt character");
}

enum getopt_idx {
	GETOPT_IDX_CMD_NAME,
	GETOPT_IDX_OPTION1,
	GETOPT_IDX_OPTION2,
	GETOPT_IDX_OPTARG
};

static void test_getopt(void)
{
	const struct shell *shell = shell_backend_dummy_get_ptr();
	struct z_getopt_state *state;
	const char *test_opts = "ac:";
	char *argv[] = {
		[GETOPT_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_IDX_OPTION1] = "-a",
		[GETOPT_IDX_OPTION2] = "-c",
		[GETOPT_IDX_OPTARG] = "foo",
	};
	int argc = ARRAY_SIZE(argv);
	int c;

	z_shell_getopt_init(&shell->ctx->getopt_state);
	/* Test uknown option */

	c = shell_getopt(shell, argc, argv, test_opts);
	zassert_equal(c, 'a', "unexpected opt character");
	c = shell_getopt(shell, argc, argv, test_opts);
	zassert_equal(c, 'c', "unexpected opt character");

	c = shell_getopt(shell, argc, argv, test_opts);
	state = shell_getopt_state_get(shell);
	zassert_equal(0, strcmp(argv[GETOPT_IDX_OPTARG], state->optarg),
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
	const struct shell *shell = shell_backend_dummy_get_ptr();
	struct z_getopt_state *state;
	int verbose_flag = 0;
	/* getopt_long stores the option index here. */
	int option_index = 0;
	int c;
	struct z_option long_options[] = {
		/* These options set a flag. */
		{"verbose",	no_argument,		&verbose_flag, 1},
		{"brief",	no_argument,		&verbose_flag, 0},
		/* These options don’t set a flag.
		   We distinguish them by their indices. */
		{"add",		no_argument,		0, 'a'},
		{"create",	required_argument,	0, 'c'},
		{"delete",	required_argument,	0, 'd'},
		{"long",	required_argument,	0, 'e'},
		{0, 0, 0, 0}
	};
	const char *accepted_opt = "ac:d:e:";

	char *argv1[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--verbose",
		[GETOPT_LONG_IDX_OPT] = "--create",
		[GETOPT_LONG_IDX_OPTARG] = "some_file",
	};
	int argc1 = ARRAY_SIZE(argv1);

	char *argv2[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		[GETOPT_LONG_IDX_OPT] = "-d",
		[GETOPT_LONG_IDX_OPTARG] = "other_file",
	};
	int argc2 = ARRAY_SIZE(argv2);

	char *argv3[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		[GETOPT_LONG_IDX_OPT] = "-a",
	};
	int argc3 = ARRAY_SIZE(argv3);

	/* this test distinguish getopt_long and getopt_long_only functions */
	char *argv4[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		/* below should not be interpreted as "--long/-e" option */
		[GETOPT_LONG_IDX_OPT] = "-l",
		[GETOPT_LONG_IDX_OPTARG] = "long_argument",
	};
	int argc4 = ARRAY_SIZE(argv4);

	/* Test scenario 1 */
	z_shell_getopt_init(&shell->ctx->getopt_state);
	c = shell_getopt_long(shell, argc1, argv1, accepted_opt,
			      long_options, &option_index);
	zassert_equal(verbose_flag, 1, "verbose flag expected");
	c = shell_getopt_long(shell, argc1, argv1, accepted_opt,
			      long_options, &option_index);
	state = shell_getopt_state_get(shell);
	zassert_equal('c', c, "unexpected option");
	zassert_equal(0, strcmp(state->optarg, argv1[GETOPT_LONG_IDX_OPTARG]),
		      "unexpected optarg");
	c = shell_getopt_long(shell, argc1, argv1, accepted_opt,
			      long_options, &option_index);
	zassert_equal(-1, c, "shell_getopt_long shall return -1");

	/* Test scenario 2 */
	z_shell_getopt_init(&shell->ctx->getopt_state);
	state = shell_getopt_state_get(shell);
	c = shell_getopt_long(shell, argc2, argv2, accepted_opt,
			      long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = shell_getopt_long(shell, argc2, argv2, accepted_opt,
			      long_options, &option_index);
	state = shell_getopt_state_get(shell);
	zassert_equal('d', c, "unexpected option");
	zassert_equal(0, strcmp(state->optarg, argv2[GETOPT_LONG_IDX_OPTARG]),
		      "unexpected optarg");
	c = shell_getopt_long(shell, argc2, argv2, accepted_opt,
			      long_options, &option_index);
	zassert_equal(-1, c, "shell_getopt_long shall return -1");

	/* Test scenario 3 */
	z_shell_getopt_init(&shell->ctx->getopt_state);
	state = shell_getopt_state_get(shell);
	c = shell_getopt_long(shell, argc3, argv3, accepted_opt,
			      long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = shell_getopt_long(shell, argc3, argv3, accepted_opt,
			      long_options, &option_index);
	state = shell_getopt_state_get(shell);
	zassert_equal('a', c, "unexpected option");
	c = shell_getopt_long(shell, argc3, argv3, accepted_opt,
			      long_options, &option_index);
	zassert_equal(-1, c, "shell_getopt_long shall return -1");

	/* Test scenario 4 */
	z_shell_getopt_init(&shell->ctx->getopt_state);
	state = shell_getopt_state_get(shell);
	c = shell_getopt_long(shell, argc4, argv4, accepted_opt,
			      long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = shell_getopt_long(shell, argc4, argv4, accepted_opt,
			      long_options, &option_index);
	state = shell_getopt_state_get(shell);

	/* Function was called with option '-l'. It is expected it will be
	 * NOT evaluated to '--long' which has flag 'e'.
	 */
	zassert_not_equal('e', c, "unexpected option match");
	c = shell_getopt_long(shell, argc4, argv4, accepted_opt,
			      long_options, &option_index);
}

static void test_getopt_long_only(void)
{
	/* Below test is based on example
	 * https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Option-Example.html
	 */
	const struct shell *shell = shell_backend_dummy_get_ptr();
	struct z_getopt_state *state;
	int verbose_flag = 0;
	/* getopt_long stores the option index here. */
	int option_index = 0;
	int c;
	struct z_option long_options[] = {
		/* These options set a flag. */
		{"verbose",	no_argument,		&verbose_flag, 1},
		{"brief",	no_argument,		&verbose_flag, 0},
		/* These options don’t set a flag.
		   We distinguish them by their indices. */
		{"add",		no_argument,		0, 'a'},
		{"create",	required_argument,	0, 'c'},
		{"delete",	required_argument,	0, 'd'},
		{"long",	required_argument,	0, 'e'},
		{0, 0, 0, 0}
	};
	const char *accepted_opt = "ac:d:e:";

	char *argv1[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--verbose",
		[GETOPT_LONG_IDX_OPT] = "--create",
		[GETOPT_LONG_IDX_OPTARG] = "some_file",
	};
	int argc1 = ARRAY_SIZE(argv1);

	char *argv2[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		[GETOPT_LONG_IDX_OPT] = "-d",
		[GETOPT_LONG_IDX_OPTARG] = "other_file",
	};
	int argc2 = ARRAY_SIZE(argv2);

	char *argv3[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		[GETOPT_LONG_IDX_OPT] = "-a",
	};
	int argc3 = ARRAY_SIZE(argv3);

	/* this test distinguish getopt_long and getopt_long_only functions */
	char *argv4[] = {
		[GETOPT_LONG_IDX_CMD_NAME] = "cmd_name",
		[GETOPT_LONG_IDX_VERBOSE] = "--brief",
		/* below should be interpreted as "--long/-e" option */
		[GETOPT_LONG_IDX_OPT] = "-l",
		[GETOPT_LONG_IDX_OPTARG] = "long_argument",
	};
	int argc4 = ARRAY_SIZE(argv4);

	/* Test scenario 1 */
	z_shell_getopt_init(&shell->ctx->getopt_state);
	c = shell_getopt_long_only(shell, argc1, argv1, accepted_opt,
				   long_options, &option_index);
	zassert_equal(verbose_flag, 1, "verbose flag expected");
	c = shell_getopt_long_only(shell, argc1, argv1, accepted_opt,
				   long_options, &option_index);
	state = shell_getopt_state_get(shell);
	zassert_equal('c', c, "unexpected option");
	zassert_equal(0, strcmp(state->optarg, argv1[GETOPT_LONG_IDX_OPTARG]),
		      "unexpected optarg");
	c = shell_getopt_long_only(shell, argc1, argv1, accepted_opt,
				   long_options, &option_index);
	zassert_equal(-1, c, "shell_getopt_long_only shall return -1");

	/* Test scenario 2 */
	z_shell_getopt_init(&shell->ctx->getopt_state);
	state = shell_getopt_state_get(shell);
	c = shell_getopt_long_only(shell, argc2, argv2, accepted_opt,
				   long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = shell_getopt_long_only(shell, argc2, argv2, accepted_opt,
				   long_options, &option_index);
	state = shell_getopt_state_get(shell);
	zassert_equal('d', c, "unexpected option");
	zassert_equal(0, strcmp(state->optarg, argv2[GETOPT_LONG_IDX_OPTARG]),
		      "unexpected optarg");
	c = shell_getopt_long_only(shell, argc2, argv2, accepted_opt,
				   long_options, &option_index);
	zassert_equal(-1, c, "shell_getopt_long_only shall return -1");

	/* Test scenario 3 */
	z_shell_getopt_init(&shell->ctx->getopt_state);
	state = shell_getopt_state_get(shell);
	c = shell_getopt_long_only(shell, argc3, argv3, accepted_opt,
				   long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = shell_getopt_long_only(shell, argc3, argv3, accepted_opt,
				   long_options, &option_index);
	state = shell_getopt_state_get(shell);
	zassert_equal('a', c, "unexpected option");
	c = shell_getopt_long_only(shell, argc3, argv3, accepted_opt,
				   long_options, &option_index);
	zassert_equal(-1, c, "shell_getopt_long_only shall return -1");

	/* Test scenario 4 */
	z_shell_getopt_init(&shell->ctx->getopt_state);
	state = shell_getopt_state_get(shell);
	c = shell_getopt_long_only(shell, argc4, argv4, accepted_opt,
				   long_options, &option_index);
	zassert_equal(verbose_flag, 0, "verbose flag expected");
	c = shell_getopt_long_only(shell, argc4, argv4, accepted_opt,
				   long_options, &option_index);
	state = shell_getopt_state_get(shell);

	/* Function was called with option '-l'. It is expected it will be
	 * evaluated to '--long' which has flag 'e'.
	 */
	zassert_equal('e', c, "unexpected option");
	c = shell_getopt_long_only(shell, argc4, argv4, accepted_opt,
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
