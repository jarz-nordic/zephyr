/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef POT_H_
#define POT_H_

#include <stdint.h>
#include <pot_element.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup libpot Partially Ordered Tree Library
 * @{
 * @brief Implementation of partially ordered trees with non-defined element.
 */

#ifndef POT_LOG_DEBUG
#define POT_LOG_DEBUG(...)
#endif

/**
 * @brief Function for initializing the tree
 *
 * @param p_array Pointer to the array of the pointers to the tree nodes.
 * @param size    Size of the array.
 */
void pot_init(struct pot_element **p_array, uint32_t size);

/**
 * @brief Function for adding node to the tree.
 *
 * @param p_elem Pointer to the element to be added as a node.
 *
 * @retval 0	   Success.
 * @retval -ENOMEM No space.
 */
int pot_push(struct pot_element *p_elem);

/**
 * @brief Function for getting root node of the tree.
 *
 * @warning Root node is removed from the tree as a consequence.
 *
 * @return Pointer to the root node of the tree.
 */
struct pot_element *pot_pop(void);

/**
 * @brief Function for peeking at the root node of the tree.
 *
 * @note Node is not removed from the tree.
 *
 * @return Pointer to the root node of the tree.
 */
struct pot_element *pot_get(void);

/**
 * @brief Function for removing node from the tree.
 *
 * @retval 0  Success.
 * @retval -EAGAIN No node in tree.
 */
int pot_remove(struct pot_element *p_elem);

/**
 *@}
 **/

#ifdef __cplusplus
}
#endif

#endif // POT_H_
