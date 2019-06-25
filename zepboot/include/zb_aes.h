/*
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef H_ZB_AES_
#define H_ZB_AES_

#include <sys/types.h>

#define AES_BLOCK_SIZE 16

#ifdef __cplusplus
extern "C" {
#endif

/** @brief aes API
 * @{
 */

/**
 * @brief zb_aes_ctr_mode
 *
 * perform aes ctr calculation
 *
 * @param buf pointer to buffer to encrypt / encrypted buffer
 * @param len bytes to encrypt
 * @param ctr counter (as byte array)
 * @param key encryption key
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_aes_ctr_mode(u8_t *buf, size_t len, u8_t *ctr, const u8_t *key);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif
