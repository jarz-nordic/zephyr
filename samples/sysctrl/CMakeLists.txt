#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project("SYSCTRL application")

# Include application event headers
zephyr_library_include_directories(src/events)
zephyr_library_include_directories(src/modules/messaging)

# Application sources
# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)

target_sources(app PRIVATE
		src/events/clock_event.c
		src/events/status_event.c
		src/events/led_event.c
		src/events/gpms_event.c
		src/events/gpms_notify_event.c
		src/events/sleep_event.c
		src/events/performance_event.c
		src/events/prism_event.c
		src/events/ld_notify_event.c
		src/events/radio_event.c
)

target_sources(app PRIVATE
		src/modules/dummy/leds.c
		src/modules/power_management/gpms.c
		src/modules/power_management/power_control.c
		src/modules/messaging/filter.c
		src/modules/messaging/prism_dispatcher.c
		src/modules/messaging/ncm.c
)
# NORDIC SDK APP END
