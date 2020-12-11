/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <unity.h>
#include <stdbool.h>

#include "../../filter.c"

#include <internal/services/nrfs_led.h>
#include <internal/services/nrfs_gpms.h>
#include <internal/services/nrfs_mts.h>
#include <internal/services/nrfs_temp.h>

#include "mock_ncm.h"
#include "mock_event_manager.h"
#include "mock_prism_event.h"
#include "mock_status_event.h"
#include "mock_led_event.h"
#include "mock_gpms_event.h"
#include "mock_mts_event.h"
#include "mock_temp_event.h"

void setUp(void)
{
}

void test_msg_reject(void)
{
	static const uint8_t expected_alloc_size = 0;
	int tmp;
	nrfs_phy_t msg;


	__wrap_ncm_fill_CMockExpectAnyArgs(__LINE__);
	__wrap_ncm_alloc_CMockExpectAndReturn(__LINE__,
					      expected_alloc_size, &tmp);
	__wrap_ncm_notify_CMockExpectAnyArgs(__LINE__);

	msg_reject(&msg);
}

void test_msg_received(void)
{
	struct prism_event evt;
	uint8_t buffer[10];
	nrfs_phy_t msg;
	int tmp;

	msg.p_buffer = buffer;
	evt.p_msg = &msg;
	memset(buffer, 0xFF, sizeof(buffer)); // set uncrecognized service

	/* Case 1.Service not recognized */
	__wrap_prism_event_release_CMockExpect(__LINE__, &msg);
	msg_received(&evt);

	/* Case 2. service: led */
	/* service rejected */
	__wrap_prism_event_release_CMockExpect(__LINE__, &msg);
	NRFS_SERVICE_HDR_FILL(((nrfs_led_t *)msg.p_buffer), NRFS_LED_REQ_CHANGE_STATE);

	__wrap_ncm_fill_CMockExpectAnyArgs(__LINE__);
	__wrap_ncm_alloc_CMockIgnoreAndReturn(__LINE__, &tmp);
	__wrap_ncm_notify_CMockExpectAnyArgs(__LINE__);
	msg_received(&evt);

	/* service accepted */
	struct led_event led_evt;
	NRFS_SERVICE_HDR_FILL(((nrfs_led_t *)msg.p_buffer), NRFS_LED_REQ_CHANGE_STATE);
	__wrap__event_submit_CMockExpectAnyArgs(__LINE__);
	__wrap_new_led_event_CMockIgnoreAndReturn(__LINE__, &led_evt);
	msg_received(&evt);

	/* Case 3. service gpsm */
	struct gpms_event gpms_evt;
	NRFS_SERVICE_HDR_FILL(((nrfs_gpms_radio_t *)msg.p_buffer), NRFS_GPMS_REQ_RADIO);
	__wrap__event_submit_CMockExpectAnyArgs(__LINE__);
	__wrap_new_gpms_event_CMockIgnoreAndReturn(__LINE__, &gpms_evt);
	msg_received(&evt);

	/* Case 4. service mts */
	struct mts_event mts_evt;
	NRFS_SERVICE_HDR_FILL(((nrfs_mts_copy_t *)msg.p_buffer), NRFS_MTS_REQ_COPY);
	__wrap__event_submit_CMockExpectAnyArgs(__LINE__);
	__wrap_new_mts_event_CMockIgnoreAndReturn(__LINE__, &mts_evt);
	msg_received(&evt);

	/* Case 5. service temp */
	struct temp_event temp_evt;
	NRFS_SERVICE_HDR_FILL(((nrfs_temp_measure_t *)msg.p_buffer), NRFS_TEMP_REQ_SUBSCRIBE);
	__wrap__event_submit_CMockExpectAnyArgs(__LINE__);
	__wrap_new_temp_event_CMockIgnoreAndReturn(__LINE__, &temp_evt);
	msg_received(&evt);

}

void test_event_handler(void)
{
	bool ret;
	struct event_header eh;

	__wrap_is_status_event_CMockExpectAndReturn(__LINE__, &eh, false);
	__wrap_is_prism_event_CMockExpectAndReturn(__LINE__, &eh, false);
	ret = event_handler(&eh);

	TEST_ASSERT_EQUAL(ret, false);
}

/* It is required to be added to each test. That is because unity is using
 * different main signature (returns int) and zephyr expects main which does
 * not return value.
 */
extern int unity_main(void);

void main(void)
{
	(void)unity_main();
}
