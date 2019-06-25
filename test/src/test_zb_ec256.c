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
#include "../../zepboot/include/zb_ec256.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(test_zb_ec256);

extern u8_t test_msg[];
extern u8_t test_msg_hash[];
extern u32_t test_msg_crc32;

/**
 * @brief Test hash calculation for data in flash
 */
void test_zb_hash_flash(void)
{
	int err, cnt;
	struct zb_slt_area area;
	u8_t hash[HASH_BYTES];

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0,  "Unable to get slotarea count: [cnt %d]", cnt);

	err = zb_slt_area_get(&area, 0);
	zassert_true(err == 0,  "Unable to get slotarea info: [err %d]", err);

	err = zb_flash_erase(area.slt0_fldev, area.slt0_offset, area.slt0_size);
	zassert_true(err == 0,  "Unable to erase image 0 area: [err %d]", err);

	err = zb_flash_write(area.slt0_fldev, area.slt0_offset, test_msg,
			     HASH_BYTES);
	zassert_true(err == 0,  "Unable to write test message: [err %d]", err);

	err = zb_hash_flash(hash, area.slt0_fldev, area.slt0_offset,
			    HASH_BYTES);
	zassert_true(err == 0, "Hash calculation failed: [err %d]", err);

	err = memcmp(hash, test_msg_hash, HASH_BYTES);
	zassert_true(err == 0, "Hash differs");
}

extern u8_t test_signature[];

/**
 * @brief Test signature verification
 */
void test_zb_sign_verify(void)
{
	int err;
	u8_t tmp;

	err = zb_sign_verify(test_msg_hash, test_signature);
	zassert_true(err == 0, "Signature validation failed: [err %d]", err);

	/* modify the key */
	tmp = test_msg_hash[0];
	test_msg_hash[0] >>=1;
	err = zb_sign_verify(test_msg_hash, test_signature);
	zassert_false(err == 0, "Invalid hash generates valid signature");

	/* reset the key */
	test_msg_hash[0] = tmp;

}

extern u8_t test_enc_pub_key[];
extern u8_t test_enc_key[];

/**
 * @brief Test the generation of an encryption key from public key
 */
void test_zb_get_encr_key(void)
{
	int err;
	u8_t enc_key[16], tmp;

	err = zb_get_encr_key(enc_key, test_enc_pub_key, 16);
	zassert_true(err == 0,  "Failed to get encryption key: [err %d]", err);
	err = memcmp(enc_key, test_enc_key, 16);
	zassert_true(err == 0,  "Encryption keys differ: [err %d]", err);

	/* modify the key */
	tmp = test_enc_pub_key[0];
	test_enc_pub_key[0] >>= 1;
	err = zb_get_encr_key(enc_key, test_enc_pub_key, 16);
	zassert_false(err == 0,  "Invalid pubkey generates encryption key");

	/* reset the key */
	test_enc_pub_key[0] = tmp;

}

/**
 * @brief Test the generation of an encryption key from public key
 */
void test_zb_crc32(void)
{
	int err, cnt;
	struct zb_slt_area area;
	u32_t crc32;

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0,  "Unable to get slotarea count: [cnt %d]", cnt);

	err = zb_slt_area_get(&area, 0);
	zassert_true(err == 0,  "Unable to get slotarea info: [err %d]", err);

	err = zb_flash_erase(area.slt0_fldev, area.slt0_offset, area.slt0_size);
	zassert_true(err == 0,  "Unable to erase image 0 area: [err %d]", err);

	err = zb_flash_write(area.slt0_fldev, area.slt0_offset, test_msg,
			     HASH_BYTES);
	zassert_true(err == 0,  "Unable to write test message: [err %d]", err);

	err = zb_crc32_flash(&crc32, area.slt0_fldev, area.slt0_offset,
			    HASH_BYTES);
	zassert_true(err == 0, "CRC32 calculation failed: [err %d]", err);

	err = crc32 - test_msg_crc32;
	zassert_true(err == 0, "CRC32 differs");

}

void test_zb_ec256(void)
{
	ztest_test_suite(test_zb_ec256,
			 ztest_unit_test(test_zb_hash_flash),
			 ztest_unit_test(test_zb_sign_verify),
			 ztest_unit_test(test_zb_get_encr_key),
			 ztest_unit_test(test_zb_crc32)
	);

	ztest_run_test_suite(test_zb_ec256);
}
