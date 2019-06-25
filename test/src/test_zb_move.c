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
#include "../../zepboot/include/zb_move.h"


#include <logging/log.h>
LOG_MODULE_REGISTER(test_zb_move);

extern const unsigned char test_image_slt0[1536];
extern const unsigned char test_image_slt1[1536];
extern const unsigned char test_image_slt0_enc[1536];
extern const unsigned char test_image_slt1_enc[1536];

#define HDR_SIZE 512
/**
 * @brief Test the classic unencrypted move
 */
void test_zb_image_classic_move_clr(void)
{
	int err, cnt;
	struct zb_slt_area area;
	struct zb_cmd cmd;
	u8_t imgheader[HDR_SIZE];
	u8_t img[1536-HDR_SIZE];

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0, "Unable to get slotarea count: [cnt %d]", cnt);
	err = zb_slt_area_get(&area, 0);
	zassert_true(err == 0, "Unable to get slotarea info: [err %d]", err);

	err = zb_flash_erase(area.slt1_fldev, area.slt1_offset, area.slt1_size);
	zassert_true(err == 0, "Unable to erase image 1 area: [err %d]", err);

	err = zb_flash_write(area.slt1_fldev, area.slt1_offset, test_image_slt0,
			     sizeof(test_image_slt0));
	zassert_true(err == 0, "Unable to write image data: [err %d]", err);

	err = zb_flash_erase(area.swpstat_fldev, area.swpstat_offset,
			     area.swpstat_size);
	zassert_true(err == 0, "Unable to erase swpstat area: [err %d]", err);
	cmd.cmd1 = 0;
	cmd.cmd2 = CMD2_SWP_START;
	cmd.cmd3 = 0x0;

	err = zb_cmd_write_swpstat(&area, &cmd);
	zassert_true(err == 0, "Failed to write command");

	err = zb_img_swap(&area);

	err = zb_flash_read(area.slt0_fldev, area.slt0_offset,
			    imgheader, HDR_SIZE);
	zassert_true(err == 0, "Unable to read moved header");
	err = memcmp(imgheader, test_image_slt0, HDR_SIZE);
	zassert_true(err == 0, "Difference detected in moved header");

	err = zb_flash_read(area.slt0_fldev, area.slt0_offset + HDR_SIZE,
			    img, 1536 - HDR_SIZE);
	zassert_true(err == 0, "Unable to read moved image");
	err = memcmp(img, &test_image_slt0[HDR_SIZE], 1536 - HDR_SIZE);
	zassert_true(err == 0, "Difference detected in image");
}

/**
 * @brief Test the classic encrypted move
 */
void test_zb_image_classic_move_enc(void)
{
	int err, cnt;
	struct zb_slt_area area;
	struct zb_cmd cmd;
	u8_t imgheader[HDR_SIZE];
	u8_t img[1536-HDR_SIZE];

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0, "Unable to get slotarea count: [cnt %d]", cnt);
	err = zb_slt_area_get(&area, 0);
	zassert_true(err == 0, "Unable to get slotarea info: [err %d]", err);

	err = zb_flash_erase(area.slt1_fldev, area.slt1_offset, area.slt1_size);
	zassert_true(err == 0, "Unable to erase image 1 area: [err %d]", err);

	err = zb_flash_write(area.slt1_fldev, area.slt1_offset,
			     test_image_slt0_enc, sizeof(test_image_slt0_enc));
	zassert_true(err == 0, "Unable to write image data: [err %d]", err);

	err = zb_flash_erase(area.swpstat_fldev, area.swpstat_offset,
			     area.swpstat_size);
	zassert_true(err == 0, "Unable to erase swpstat area: [err %d]", err);
	cmd.cmd1 = 0;
	cmd.cmd2 = CMD2_SWP_START;
	cmd.cmd3 = 0x0;

	err = zb_cmd_write_swpstat(&area, &cmd);
	zassert_true(err == 0, "Failed to write command");

	err = zb_img_swap(&area);

	err = zb_flash_read(area.slt0_fldev, area.slt0_offset,
			    imgheader, HDR_SIZE);
	zassert_true(err == 0, "Unable to read moved header");
	err = memcmp(imgheader, test_image_slt0_enc, HDR_SIZE);
	zassert_true(err == 0, "Difference detected in moved header");

	err = zb_flash_read(area.slt0_fldev, area.slt0_offset + HDR_SIZE,
			    img, 1536 - HDR_SIZE);
	zassert_true(err == 0, "Unable to read moved image");
	/* test_image_slt0 contains unencrypted test_image_slt0_enc */
	err = memcmp(img, &test_image_slt0[HDR_SIZE], 1536 - HDR_SIZE);
	zassert_true(err == 0, "Difference detected in image");
}

/**
 * @brief Test the inplace unencrypted move
 */
void test_zb_image_inplace_move_clr(void)
{
	int err, cnt;
	struct zb_slt_area area;
	struct zb_cmd cmd;
	u8_t imgheader[HDR_SIZE];
	u8_t img[1536-HDR_SIZE];

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0, "Unable to get slotarea count: [cnt %d]", cnt);
	err = zb_slt_area_get(&area, 0);
	zassert_true(err == 0, "Unable to get slotarea info: [err %d]", err);

	err = zb_flash_erase(area.slt1_fldev, area.slt1_offset, area.slt1_size);
	zassert_true(err == 0, "Unable to erase image 1 area: [err %d]", err);

	err = zb_flash_write(area.slt1_fldev, area.slt1_offset, test_image_slt1,
			     sizeof(test_image_slt1));
	zassert_true(err == 0, "Unable to write image data: [err %d]", err);

	err = zb_flash_erase(area.swpstat_fldev, area.swpstat_offset,
			     area.swpstat_size);
	zassert_true(err == 0, "Unable to erase swpstat area: [err %d]", err);
	cmd.cmd1 = 0;
	cmd.cmd2 = CMD2_SWP_START | CMD2_MASK_INPLACE;
	cmd.cmd3 = 0x0;

	err = zb_cmd_write_swpstat(&area, &cmd);
	zassert_true(err == 0, "Failed to write command");

	err = zb_img_swap(&area);

	err = zb_flash_read(area.slt1_fldev, area.slt1_offset,
			    imgheader, HDR_SIZE);
	zassert_true(err == 0, "Unable to read moved header");
	err = memcmp(imgheader, test_image_slt1, HDR_SIZE);
	zassert_true(err == 0, "Difference detected in moved header");

	err = zb_flash_read(area.slt1_fldev, area.slt1_offset + HDR_SIZE,
			    img, 1536 - HDR_SIZE);
	zassert_true(err == 0, "Unable to read moved image");
	err = memcmp(img, &test_image_slt1[HDR_SIZE], 1536 - HDR_SIZE);
	zassert_true(err == 0, "Difference detected in image");
}

/**
 * @brief Test the in place encrypted move
 */
void test_zb_image_inplace_move_enc(void)
{
	int err, cnt;
	struct zb_slt_area area;
	struct zb_cmd cmd;
	u8_t imgheader[HDR_SIZE];
	u8_t img[1536-HDR_SIZE];

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0, "Unable to get slotarea count: [cnt %d]", cnt);
	err = zb_slt_area_get(&area, 0);
	zassert_true(err == 0, "Unable to get slotarea info: [err %d]", err);

	err = zb_flash_erase(area.slt1_fldev, area.slt1_offset, area.slt1_size);
	zassert_true(err == 0, "Unable to erase image 1 area: [err %d]", err);

	err = zb_flash_write(area.slt1_fldev, area.slt1_offset,
			     test_image_slt1_enc, sizeof(test_image_slt1_enc));
	zassert_true(err == 0, "Unable to write image data: [err %d]", err);

	err = zb_flash_erase(area.swpstat_fldev, area.swpstat_offset,
			     area.swpstat_size);
	zassert_true(err == 0, "Unable to erase swpstat area: [err %d]", err);
	cmd.cmd1 = 0;
	cmd.cmd2 = CMD2_SWP_START | CMD2_MASK_INPLACE;
	cmd.cmd3 = 0x0;

	err = zb_cmd_write_swpstat(&area, &cmd);
	zassert_true(err == 0, "Failed to write command");

	err = zb_img_swap(&area);

	err = zb_flash_read(area.slt1_fldev, area.slt1_offset,
			    imgheader, HDR_SIZE);
	zassert_true(err == 0, "Unable to read moved header");
	err = memcmp(imgheader, test_image_slt1_enc, HDR_SIZE);
	zassert_true(err == 0, "Difference detected in moved header");

	err = zb_flash_read(area.slt1_fldev, area.slt1_offset + HDR_SIZE,
			    img, 1536 - HDR_SIZE);
	zassert_true(err == 0, "Unable to read moved image");
	err = memcmp(img, &test_image_slt1[HDR_SIZE], 1536 - HDR_SIZE);
	zassert_true(err == 0, "Difference detected in image");

}

void test_zb_move(void)
{
	ztest_test_suite(test_zb_move,
			 ztest_unit_test(test_zb_image_classic_move_clr),
			 ztest_unit_test(test_zb_image_classic_move_enc),
			 ztest_unit_test(test_zb_image_inplace_move_clr),
			 ztest_unit_test(test_zb_image_inplace_move_enc)
			);

	ztest_run_test_suite(test_zb_move);
}
