/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <errno.h>
#include <flash.h>
#include "../../zepboot/include/zb_flash.h"
#include "../../zepboot/include/zb_tlv.h"
#include "../../zepboot/include/zb_image.h"
#include "../../zepboot/include/zb_aes.h"
#include "../../zepboot/include/zb_move.h"


#include <logging/log.h>
LOG_MODULE_REGISTER(test_zb_aes);

extern u8_t test_msg[];
extern u8_t ec256_boot_pri_key[];
u8_t enc_test_msg[HASH_BYTES];
u8_t dec_test_msg[HASH_BYTES];

/**
 * @brief Test the aes enc routine
 */
void test_zb_aes_enc(void)
{
	int err;

	u8_t ctr[AES_BLOCK_SIZE]={0};

	memcpy(enc_test_msg, test_msg, HASH_BYTES);
	err = zb_aes_ctr_mode(enc_test_msg, HASH_BYTES, ctr,
			      ec256_boot_pri_key);
	zassert_true(err == 0,  "AES CTR returned [err %d]", err);
	err = ctr[15] - HASH_BYTES/AES_BLOCK_SIZE;
	zassert_true(err == 0,  "AES CTR wrong CTR value");
	err = memcmp(enc_test_msg, test_msg, HASH_BYTES);
	zassert_false(err == 0,  "AES wrong encrypt data");

}
void test_zb_aes_dec(void)
{
	int err;

	u8_t ctr[AES_BLOCK_SIZE]={0};

	memcpy(dec_test_msg, enc_test_msg, HASH_BYTES);
	err = zb_aes_ctr_mode(dec_test_msg, HASH_BYTES, ctr,
			      ec256_boot_pri_key);
	zassert_true(err == 0,  "AES CTR returned [err %d]", err);
	err = ctr[15] - HASH_BYTES/AES_BLOCK_SIZE;
	zassert_true(err == 0,  "AES CTR wrong CTR value");
	err = memcmp(dec_test_msg, test_msg, HASH_BYTES);
	zassert_true(err == 0,  "AES wrong decrypt data");

}

void test_zb_aes(void)
{
	ztest_test_suite(test_zb_aes,
			 ztest_unit_test(test_zb_aes_enc),
			 ztest_unit_test(test_zb_aes_dec)
			);

	ztest_run_test_suite(test_zb_aes);
}
