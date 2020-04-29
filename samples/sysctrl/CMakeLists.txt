#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("Event Manager sample")

# Include application event headers
zephyr_library_include_directories(src/events)

# Application sources
# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)

target_sources(app PRIVATE
		src/events/status_event.c
		src/events/led_event.c
		src/events/read_event.c
)

target_sources(app PRIVATE
		src/modules/dummy/leds.c
		src/modules/dummy/receiver.c
)
# NORDIC SDK APP END
