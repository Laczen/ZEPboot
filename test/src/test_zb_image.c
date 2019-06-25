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

#include <logging/log.h>
LOG_MODULE_REGISTER(test_zb_image);

extern const unsigned char test_image_slt0[1536];
extern const unsigned char test_image_slt1[1536];
extern const unsigned char test_image_slt0_enc[1536];
extern const unsigned char test_image_slt1_enc[1536];

#define HDR_SIZE 512

/**
 * @brief Test the opening of a image in slot 0
 * test_image_slt0[] has been generated for load_address 0x11200
 * test_image_slt1[] has been generated for load_address 0x21200
 */
void test_zb_get_image_info_slt0(void)
{
	int err, cnt;
	struct zb_slt_area area;
	zb_img_info info;

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0,  "Unable to get slotarea count: [cnt %d]", cnt);

	err = zb_slt_area_get(&area, 0);
	zassert_true(err == 0,  "Unable to get slotarea info: [err %d]", err);

	err = zb_flash_erase(area.slt0_fldev, area.slt0_offset, area.slt0_size);
	zassert_true(err == 0,  "Unable to erase image 0 area: [err %d]", err);

	err = zb_flash_write(area.slt0_fldev, area.slt0_offset, test_image_slt0,
			     sizeof(test_image_slt0));
	zassert_true(err == 0,  "Unable to write the image: [err %d]", err);

	/* without signature verification */
	zb_img_get_info_nsc(&info, &area, false, 0, true);
	zassert_true(info.is_valid,  "Error loading image info: [err %d]", err);
	err = info.hdr_start - area.slt0_offset;
	zassert_true(err == 0,  "Wrong hdr start [%x] [%x]", info.start,
		     area.slt0_offset);
	err = info.start - area.slt0_offset - HDR_SIZE;
	zassert_true(err == 0,  "Wrong img start");
	err = info.load_address - (area.slt0_offset + HDR_SIZE);
	zassert_true(err == 0,  "Wrong load address");

	/* with signature verification */
	zb_img_get_info_wsc(&info, &area, false, 0, true);
	zassert_true(info.is_valid,  "Error loading image info: [err %d]", err);
	err = info.hdr_start - area.slt0_offset;
	zassert_true(err == 0,  "Wrong hdr start [%x] [%x]", info.start,
		     area.slt0_offset);
	err = info.start - area.slt0_offset - HDR_SIZE;
	zassert_true(err == 0,  "Wrong img start");
	err = info.load_address - (area.slt0_offset + HDR_SIZE);
	zassert_true(err == 0,  "Wrong load address");

}

/**
 * @brief Test the opening of a encrypted image in slot 0
 * test_image_slt0_enc[] has been generated for load_address 0x11200
 * test_image_slt1_enc[] has been generated for load_address 0x21200
 */
void test_zb_get_image_info_slt0_enc(void)
{
	int err, cnt;
	struct zb_slt_area area;
	zb_img_info info;

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0,  "Unable to get slotarea count: [cnt %d]", cnt);

	err = zb_slt_area_get(&area, 0);
	zassert_true(err == 0,  "Unable to get slotarea info: [err %d]", err);

	err = zb_flash_erase(area.slt0_fldev, area.slt0_offset, area.slt0_size);
	zassert_true(err == 0,  "Unable to erase image 0 area: [err %d]", err);

	err = zb_flash_write(area.slt0_fldev, area.slt0_offset,
			     test_image_slt0_enc, sizeof(test_image_slt0_enc));
	zassert_true(err == 0,  "Unable to write the image: [err %d]", err);

	/* without signature verification */
	zb_img_get_info_nsc(&info, &area, false, 0, true);
	zassert_true(info.is_valid,  "Error loading image info: [err %d]", err);
	err = info.hdr_start - area.slt0_offset;
	zassert_true(err == 0,  "Wrong hdr start [%x] [%x]", info.start,
		     area.slt0_offset);
	err = info.start - area.slt0_offset - HDR_SIZE;
	zassert_true(err == 0,  "Wrong img start");
	err = info.enc_start - area.slt0_offset - HDR_SIZE;
	zassert_true(err == 0,  "Wrong encryption start");
	err = info.load_address - (area.slt0_offset + HDR_SIZE);
	zassert_true(err == 0,  "Wrong load address");

	/* with signature verification */
	zb_img_get_info_wsc(&info, &area, false, 0, true);
	zassert_true(info.is_valid,  "Error loading image info: [err %d]", err);
	err = info.hdr_start - area.slt0_offset;
	zassert_true(err == 0,  "Wrong hdr start [%x] [%x]", info.start,
		     area.slt0_offset);
	err = info.start - area.slt0_offset - HDR_SIZE;
	zassert_true(err == 0,  "Wrong img start");
	err = info.enc_start - area.slt0_offset - HDR_SIZE;
	zassert_true(err == 0,  "Wrong encryption start");
	err = info.load_address - (area.slt0_offset + HDR_SIZE);
	zassert_true(err == 0,  "Wrong load address");

}

/**
 * @brief Test the opening of a image in slot 1
 * test_image_slt0[] has been generated for load_address 0x11200
 * test_image_slt1[] has been generated for load_address 0x21200
 */
void test_zb_get_image_info_slt1(void)
{
	int err, cnt;
	struct zb_slt_area area;
	zb_img_info info;

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0,  "Unable to get slotarea count: [cnt %d]", cnt);

	err = zb_slt_area_get(&area, 0);
	zassert_true(err == 0,  "Unable to get slotarea info: [err %d]", err);

	err = zb_flash_erase(area.slt1_fldev, area.slt1_offset, area.slt1_size);
	zassert_true(err == 0,  "Unable to erase image 0 area: [err %d]", err);

	err = zb_flash_write(area.slt1_fldev, area.slt1_offset, test_image_slt0,
			     sizeof(test_image_slt0));
	zassert_true(err == 0,  "Unable to write the image: [err %d]", err);

	/* without signature verification */
	zb_img_get_info_nsc(&info, &area, true, 0, true);
	zassert_true(info.is_valid,  "Error loading image info: [err %d]", err);
	err = info.hdr_start - area.slt1_offset;
	zassert_true(err == 0,  "Wrong hdr start [%x] [%x]", info.start,
		     area.slt1_offset);
	err = info.start - area.slt1_offset - HDR_SIZE;
	zassert_true(err == 0,  "Wrong img start");
	err = info.load_address - (area.slt0_offset + HDR_SIZE);
	zassert_true(err == 0,  "Wrong load address");

	/* with signature verification */
	zb_img_get_info_wsc(&info, &area, true, 0, true);
	zassert_true(info.is_valid,  "Error loading image info: [err %d]", err);
	err = info.hdr_start - area.slt1_offset;
	zassert_true(err == 0,  "Wrong hdr start [%x] [%x]", info.start,
		     area.slt1_offset);
	err = info.start - area.slt1_offset - HDR_SIZE;
	zassert_true(err == 0,  "Wrong img start");
	err = info.load_address - (area.slt0_offset + HDR_SIZE);
	zassert_true(err == 0,  "Wrong load address");

}

/**
 * @brief Test the opening of a encrypted image in slot 1
 * test_image_slt0[] has been generated for load_address 0x11200
 * test_image_slt1[] has been generated for load_address 0x21200
 */
void test_zb_get_image_info_slt1_enc(void)
{
	int err, cnt;
	struct zb_slt_area area;
	zb_img_info info;

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0,  "Unable to get slotarea count: [cnt %d]", cnt);

	err = zb_slt_area_get(&area, 0);
	zassert_true(err == 0,  "Unable to get slotarea info: [err %d]", err);

	err = zb_flash_erase(area.slt1_fldev, area.slt1_offset, area.slt1_size);
	zassert_true(err == 0,  "Unable to erase image 0 area: [err %d]", err);

	err = zb_flash_write(area.slt1_fldev, area.slt1_offset,
			     test_image_slt0_enc, sizeof(test_image_slt0_enc));
	zassert_true(err == 0,  "Unable to write the image: [err %d]", err);

	/* without signature verification */
	zb_img_get_info_nsc(&info, &area, true, 0, true);
	zassert_true(info.is_valid,  "Error loading image info: [err %d]", err);
	err = info.hdr_start - area.slt1_offset;
	zassert_true(err == 0,  "Wrong hdr start [%x] [%x]", info.start,
		     area.slt1_offset);
	err = info.start - area.slt1_offset - HDR_SIZE;
	zassert_true(err == 0,  "Wrong img start");
	err = info.enc_start - area.slt1_offset - HDR_SIZE;
	zassert_true(err == 0,  "Wrong encryption start");
	err = info.load_address - (area.slt0_offset + HDR_SIZE);
	zassert_true(err == 0,  "Wrong load address");

	/* with signature verification */
	zb_img_get_info_wsc(&info, &area, true, 0, true);
	zassert_true(info.is_valid,  "Error loading image info: [err %d]", err);
	err = info.hdr_start - area.slt1_offset;
	zassert_true(err == 0,  "Wrong hdr start [%x] [%x]", info.start,
		     area.slt1_offset);
	err = info.start - area.slt1_offset - HDR_SIZE;
	zassert_true(err == 0,  "Wrong img start");
	err = info.enc_start - area.slt1_offset - HDR_SIZE;
	zassert_true(err == 0,  "Wrong encryption start");
	err = info.load_address - (area.slt0_offset + HDR_SIZE);
	zassert_true(err == 0,  "Wrong load address");

}

/**
 * @brief Test the image check (this is carried out in slt1)
 * test_image_slt0[] has been generated for load_address 0x11200
 * test_image_slt1[] has been generated for load_address 0x21200
 */
void test_zb_check_image(void)
{
	int err, cnt;
	struct zb_slt_area area;
	struct zb_prm prm;
	u8_t slt;

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0,  "Unable to get slotarea count: [cnt %d]", cnt);

	err = zb_slt_area_get(&area, 0);
	zassert_true(err == 0,  "Unable to get slotarea info: [err %d]", err);

	err = zb_flash_erase(area.slt1_fldev, area.slt1_offset, area.slt1_size);
	zassert_true(err == 0,  "Unable to erase image 1 area: [err %d]", err);

	err = zb_flash_write(area.slt1_fldev, area.slt1_offset, test_image_slt0,
			     sizeof(test_image_slt0));
	zassert_true(err == 0,  "Unable to write the image: [err %d]", err);

	err = zb_img_check(&area, &slt);
	zassert_true(err == 0,  "Image check failed");
	zassert_true(slt == 0,  "Wrong slot");

	err = zb_flash_erase(area.slt1_fldev, area.slt1_offset, area.slt1_size);
	zassert_true(err == 0,  "Unable to erase image 1 area: [err %d]", err);

	err = zb_flash_write(area.slt1_fldev, area.slt1_offset, test_image_slt1,
			     sizeof(test_image_slt1));
	zassert_true(err == 0,  "Unable to write the image: [err %d]", err);

	err = zb_img_check(&area, &slt);
	zassert_true(err == 0,  "Image check failed");
	zassert_true(slt == 1,  "Wrong slot");

	prm.pri_ld_address = area.slt1_offset;
	prm.slt0_ver = 1;
	err = zb_prm_write(&area, &prm);
	zassert_true(err == 0,  "Unable to write prm area: [err %d]", err);

	err = zb_img_check(&area, &slt);
	zassert_false(err == 0,  "Image check failed");
}

void test_zb_image(void)
{
	ztest_test_suite(test_zb_image,
			 ztest_unit_test(test_zb_get_image_info_slt0),
			 ztest_unit_test(test_zb_get_image_info_slt0_enc),
			 ztest_unit_test(test_zb_get_image_info_slt1),
			 ztest_unit_test(test_zb_get_image_info_slt1_enc),
			 ztest_unit_test(test_zb_check_image)
			);

	ztest_run_test_suite(test_zb_image);
}
