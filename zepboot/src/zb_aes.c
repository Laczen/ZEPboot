/*
 * Copyright (c) 2019 LaczenJMS.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>
#include <tinycrypt/aes.h>

#include "../include/zb_aes.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(zb_aes);

int zb_aes_ctr_mode(u8_t *buf, size_t len, u8_t *ctr, const u8_t *key)
{
	struct tc_aes_key_sched_struct sched;
	u8_t buffer[AES_BLOCK_SIZE];
	u8_t nonce[AES_BLOCK_SIZE];
	u32_t i;
	u8_t blk_off, j, u8;


	(void)memcpy(nonce, ctr, sizeof(nonce));
	(void)tc_aes128_set_encrypt_key(&sched, key);
	for (i = 0; i < len; i++) {
		blk_off = i & (AES_BLOCK_SIZE - 1);
		if (blk_off == 0) {
			if (tc_aes_encrypt(buffer, nonce, &sched)) {
				for (j = AES_BLOCK_SIZE; j > 0; --j) {
                			if (++nonce[j - 1] != 0) {
                    				break;
                			}
            			}
			} else {
				return -EFAULT;
			}
		}
		/* update output */
		u8 = *buf;
		*buf++ = u8 ^ buffer[blk_off];
	}
	(void)memcpy(ctr, nonce, sizeof(nonce));

	return 0;
}
