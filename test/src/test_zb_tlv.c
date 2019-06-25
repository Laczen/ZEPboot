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

#include <logging/log.h>
LOG_MODULE_REGISTER(test_zb_tlv);

extern u8_t test_msg[];
extern u8_t test_signature[];

#define HAYSTACK_BYTES 16
u8_t test_haystack[HAYSTACK_BYTES] = {
	1,2,1,2, 	/* type = 1, length = 2, value = 1 2 */
	2,3,3,4,5, 	/* type = 2, length = 3, value = 3 4 5 */
	3,1,6,		/* type = 3, length = 1, value = 6 */
	4,2,7,8		/* type = 4, length = 1, value = 7 8 */

};

/**
 * @brief Test tlv_step method to step through a haystack in ram
 */
void test_zb_tlv_step(void)
{
	int err;
	tlv_entry entry;
	off_t offset;

	offset = 0;
	entry.type = 0;
	while ((entry.type != 1) && (offset < HAYSTACK_BYTES)) {
		zb_step_tlv(test_haystack, &offset, &entry);
	}
	zassert_true(entry.type == 1, "Entry type 1 not found");
	err = memcmp(&test_haystack[2], entry.value, entry.length);
	zassert_true(err == 0, "Entry type 1 wrong value");

	offset = 0;
	entry.type = 0;
	while ((entry.type != 2) && (offset < HAYSTACK_BYTES)) {
		zb_step_tlv(test_haystack, &offset, &entry);
	}
	zassert_true(entry.type == 2, "Entry type 2 not found");
	err = memcmp(&test_haystack[6], entry.value, entry.length);
	zassert_true(err == 0, "Entry type 2 wrong value");

	offset = 0;
	entry.type = 0;
	while ((entry.type != 3) && (offset < HAYSTACK_BYTES)) {
		zb_step_tlv(test_haystack, &offset, &entry);
	}
	zassert_true(entry.type == 3, "Entry type 3 not found");
	err = memcmp(&test_haystack[11], entry.value, entry.length);
	zassert_true(err == 0, "Entry type 3 wrong value");

	/* Look for non-existing entry */
	offset = 0;
	entry.type = 0;
	while ((entry.type != 5) && (offset < HAYSTACK_BYTES)) {
		zb_step_tlv(test_haystack, &offset, &entry);
	}
	zassert_false(entry.type == 5, "Found non-existing entry");

}

/**
 * @brief Test the opening of a tlv area without signature verification
 */
void test_zb_tlv_open_unsigned(void)
{
	int err, cnt;
	struct zb_slt_area area;
	tlv_area_hdr hdr;
	u8_t ext_haystack[sizeof(tlv_area_hdr) + HAYSTACK_BYTES];
	int ext_haystack_size;
	tlv_entry entry;
	off_t offset;

	hdr.tlva_magic = TLV_AREA_MAGIC;
	hdr.tlva_size = sizeof(ext_haystack);
	memcpy(&hdr.tlva_signature, test_signature, SIGNATURE_BYTES);

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0,  "Unable to get slotarea count: [cnt %d]", cnt);

	err = zb_slt_area_get(&area, 0);
	zassert_true(err == 0,  "Unable to get slotarea info: [err %d]", err);

	err = zb_flash_erase(area.slt0_fldev, area.slt0_offset, area.slt0_size);
	zassert_true(err == 0,  "Unable to erase image 0 area: [err %d]", err);

	memcpy(ext_haystack, &hdr, sizeof(tlv_area_hdr));
	memcpy(ext_haystack + sizeof(tlv_area_hdr), test_haystack,
	       sizeof(test_haystack));
	err = zb_flash_write(area.slt0_fldev, area.slt0_offset, ext_haystack,
			     sizeof(ext_haystack));
	zassert_true(err == 0,  "Unable to write tlva: [err %d]", err);

	ext_haystack_size = zb_open_tlv_area(area.slt0_fldev, area.slt0_offset,
					     ext_haystack, false);
	zassert_true(ext_haystack_size >= 0, "Tlv open error");
	zassert_true(ext_haystack_size == HAYSTACK_BYTES, "Tlv length error");

	offset = 0;
	while (offset < HAYSTACK_BYTES) {

		zb_step_tlv(ext_haystack, &offset, &entry);
		switch (entry.type) {
			case 1:
				err = memcmp(&test_haystack[2], entry.value,
					     entry.length);
				zassert_true(err == 0, "type 1 error");
				break;
			case 2:
				err = memcmp(&test_haystack[6], entry.value,
					     entry.length);
				zassert_true(err == 0, "type 2 error");
				break;
			case 3:
				err = memcmp(&test_haystack[11], entry.value,
					     entry.length);
				zassert_true(err == 0, "type 3 error");
				break;
		}

	}
	zassert_true(offset == HAYSTACK_BYTES, "Tlv wrong end [%d]", offset);

}

/**
 * @brief Test the opening of a tlv area with signature verification
 */
void test_zb_tlv_open_signed(void)
{
	int err, cnt;
	struct zb_slt_area area;
	tlv_area_hdr hdr;
	u8_t ext_haystack[sizeof(tlv_area_hdr) + HASH_BYTES];
	int ext_haystack_size;

	hdr.tlva_magic = TLV_AREA_MAGIC;
	hdr.tlva_size = sizeof(ext_haystack);
	memcpy(&hdr.tlva_signature, test_signature, SIGNATURE_BYTES);

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0,  "Unable to get slotarea count: [cnt %d]", cnt);

	err = zb_slt_area_get(&area, 0);
	zassert_true(err == 0,  "Unable to get slotarea info: [err %d]", err);

	err = zb_flash_erase(area.slt0_fldev, area.slt0_offset, area.slt0_size);
	zassert_true(err == 0,  "Unable to erase image 0 area: [err %d]", err);

	memcpy(ext_haystack, &hdr, sizeof(tlv_area_hdr));
	memcpy(ext_haystack + sizeof(tlv_area_hdr), test_msg, HASH_BYTES);
	err = zb_flash_write(area.slt0_fldev, area.slt0_offset, ext_haystack,
			     sizeof(ext_haystack));

	ext_haystack_size = zb_open_tlv_area(area.slt0_fldev, area.slt0_offset,
					     ext_haystack, true);
	zassert_true(ext_haystack_size >= 0, "Tlv open error");
	zassert_true(ext_haystack_size == HASH_BYTES,
		     "Incorrect tlv length");

}



void test_zb_tlv(void)
{
	ztest_test_suite(test_zb_tlv,
			 ztest_unit_test(test_zb_tlv_step),
			 ztest_unit_test(test_zb_tlv_open_unsigned),
			 ztest_unit_test(test_zb_tlv_open_signed)
			);

	ztest_run_test_suite(test_zb_tlv);
}
