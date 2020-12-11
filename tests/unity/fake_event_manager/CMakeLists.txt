#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(.)

zephyr_sources_ifdef(
  CONFIG_FAKE_EVENT_MANAGER
  event_manager.c
  )
