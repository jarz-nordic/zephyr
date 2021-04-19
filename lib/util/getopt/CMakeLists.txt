# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories_ifdef(
  CONFIG_Z_GETOPT
  .
)

zephyr_sources_ifdef(
  CONFIG_Z_GETOPT
  z_getopt.c
)

zephyr_sources_ifdef(
  CONFIG_Z_GETOPT_LONG
  z_getopt_long.c
)
