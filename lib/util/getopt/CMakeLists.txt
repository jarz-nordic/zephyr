# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories_ifdef(
  CONFIG_GETOPT
  .
)

zephyr_sources_ifdef(
  CONFIG_GETOPT
  getopt.c
  getopt_long.c
  lib_getopt_common.c
  lib_getoptvars.c
)
