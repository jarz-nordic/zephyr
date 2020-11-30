#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_shell)

# generate runner for the test
test_runner_generate(src/test_shell.c)

# create mock for module.
cmock_handle($ENV{ZEPHYR_BASE}/include/shell/shell_fprintf.h shell)
cmock_handle($ENV{ZEPHYR_BASE}/include/shell/shell_history.h shell)
cmock_handle($ENV{ZEPHYR_BASE}/include/shell/shell_log_backend.h shell)
cmock_handle($ENV{ZEPHYR_BASE}/include/shell/shell_uart.h shell)


# add test file
target_sources(app PRIVATE src/test_shell.c)
target_include_directories(app PRIVATE .)
