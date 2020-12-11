#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_filter)

# generate runner for the test
test_runner_generate(src/test_filter.c)
zephyr_library_include_directories(../fake_events)

# create mock for module.
cmock_handle($ENV{ZEPHYR_BASE}/samples/sysctrl/src/modules/messaging/ncm.h)
cmock_handle($ENV{ZEPHYR_BASE}/tests/unity/fake_event_manager/event_manager.h)

cmock_handle(../fake_events/clock_event.h)
cmock_handle(../fake_events/gpms_event.h)
cmock_handle(../fake_events/ld_notify_event.h)
cmock_handle(../fake_events/led_event.h)
cmock_handle(../fake_events/mts_event.h)
cmock_handle(../fake_events/prism_event.h)
cmock_handle(../fake_events/radio_event.h)
cmock_handle(../fake_events/status_event.h)
cmock_handle(../fake_events/temp_event.h)

target_include_directories(app PRIVATE .)

# add test file
target_sources(app PRIVATE src/test_filter.c)
