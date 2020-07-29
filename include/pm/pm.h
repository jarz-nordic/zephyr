/*
 * Copyright (c) 2020 Nordic Semuconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**
 * @file
 * @brief Power Management common header
 */

#ifndef ZEPHYR_INCLUDE_PM_PM_H_
#define ZEPHYR_INCLUDE_PM_PM_H_

#include <zephyr.h>

/** @brief Maximum number of resources. */
#define PM_RESOURCE_NUM	20

/** @brief Maximum number of modes. */
#define PM_MODE_NUM	10

/**
 * @brief Sleep modes.
 * Used to signal sleep level (or System OFF).
 */
enum pm_sleep {
	PM_SLEEP_RETENTION_HW,
	PM_SLEEP_RETENTION_SW,
	PM_SLEEP_ASSISTED,
	PM_SLEEP_SYSTEM_OFF,
};

/**
 * @brief Request types.
 * Used to signal request type.
 */
enum pm_request_type {
	PM_REQUEST_SLEEP,
	PM_REQUEST_PERFORMANCE,
};

/**
 * @brief Response type.
 * Response type that signals how the request was handled.
 */
enum pm_response {
	PM_RESPONSE_OK,		/* Request will be fulfilled */
	PM_RESPONSE_IMPOSSIBLE, /* Request cannot be fulfilled */
	PM_RESPONSE_NONE,	/* No response at this time - either no required or delayed */
	PM_RESPONSE_ERROR,	/* Error during request handling */
};

/** 
 * @brief Sleep request.
 * Sleep request, coming from local domains when sleep mode or system off is needed.
 */
struct pm_request_sleep {
	uint8_t       domain;
	enum pm_sleep state;	/* Requested sleep state. */
	uint64_t      time;	/* Expected wake-up time (only for sleep states). */
	uint32_t      latency;	/* Maximum tolerated interrupt latency. */
};

/** 
 * @brief Performance request.
 * Performance request, coming from local domains when RADIO or frequency is needed.
 */
/* TODO: change to based on local PM instead of copy of power_request.
 * Question is how to combine RADIO requests and frequency requests into one. */
struct pm_request_performance {
	uint8_t  domain;
	uint64_t time;
	uint8_t  resource;
	uint8_t  mode;
};

/**
 * @brief Power request.
 * Power request, internal structure compiled from sleep and performance requests.
 */
struct pm_request_power {
	uint8_t  domain;
	uint64_t time;
	uint8_t  resource;
	uint8_t  mode;
};

/**
 * @brief Return structure.
 * Return structure combines requested resource with response.
 */
struct pm_return {
	enum pm_response response;
	uint8_t          domain;
	uint8_t          resource;
	uint8_t          mode;
};

/**
 * @brief State structure.
 * State structure is array of resources that stores their modes.
 */
struct pm_state {
	uint8_t res[PM_RESOURCE_NUM];
};

/**
 * @brief Traversal structure.
 * Traversal structure combines return code with subsequent state and required time.
 */
struct pm_traversal {
	struct pm_state  state;		/* State after request. */
	enum pm_response response;	/* Traversal return code. */
	uint64_t         time;		/* Required time (valid for OK response code). */
};

#endif /* ZEPHYR_INCLUDE_PM_PM_H_ */
