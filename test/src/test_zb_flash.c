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

#include <logging/log.h>
LOG_MODULE_REGISTER(test_zb_flash);

/**
 * @brief Test zb_get_area()
 */
void test_zb_get_area(void)
{
	int err, cnt;
	struct zb_slt_area area;

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0,  "Unable to get slotarea count: [cnt %d]", cnt);

	while (cnt > 0) {
		err = zb_slt_area_get(&area, cnt - 1);
		zassert_true(err == 0,  "Unable to get slotarea info: [err %d]",
			     err);
		cnt--;
	}
}

/**
 * @brief Test read and write of zb commands
 */
void test_zb_cmd(void)
{
	int err, cnt;
	struct zb_slt_area area;
	struct zb_cmd cmd, cmd_rd;

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0,  "Unable to get slotarea count: [cnt %d]", cnt);

	err = zb_slt_area_get(&area, 0);
	zassert_true(err == 0,  "Unable to get slotarea info: [err %d]", err);

	err = zb_erase_swpstat(&area);
	zassert_true(err == 0,  "Unable to erase stat area: [err %d]", err);

	err = zb_cmd_read_swpstat(&area, &cmd_rd);
	zassert_true(err = -ENOENT, "Found cmd entry in empty flash area");

	cmd.cmd1 = 0x0;
	cmd.cmd2 = 0x0;
	cmd.cmd3 = 0x0;

	err = zb_cmd_write_swpstat(&area, &cmd);
	zassert_true(err == 0, "Failed to write command");

	err = zb_cmd_read_swpstat(&area, &cmd_rd);
	zassert_true(((cmd.cmd1 == cmd_rd.cmd1) && (cmd.cmd2 == cmd_rd.cmd2) &&
		     (cmd.cmd3 == cmd_rd.cmd3)),
		      "Found different cmd entry in flash area");

	/* write commands until stat area is full */
	while (err == 0) {
		cmd.cmd3++;
		err = zb_cmd_write_swpstat(&area, &cmd);
		if (cmd.cmd3 > 1024) {
			break;
		}
	}
	zassert_true(err == -ENOSPC, "To many cmd writes possible");
}

/**
 * @brief Test read and write of zb image parameters
 */
void test_zb_prm(void)
{
	int err, cnt;
	struct zb_slt_area area;
	struct zb_prm prm, prm_rd;

	cnt = zb_slt_area_cnt();
	zassert_false(cnt == 0,  "Unable to get slotarea count: [cnt %d]", cnt);

	err = zb_slt_area_get(&area, 0);
	zassert_true(err == 0,  "Unable to get slotarea info: [err %d]", err);

	err = zb_erase_slt0end(&area);
	zassert_true(err == 0,  "Unable to erase stat area: [err %d]", err);

	prm.slt0_crc32 = 0x0;
	prm.slt0_ver = 0x0;
	prm.pri_ld_address = 0x0;
	prm.slt1_crc32 = 0x0;
	prm.slt1_ver = 0x0;
	prm.sec_ld_address = 0x0;

	err = zb_prm_write(&area, &prm);
	zassert_true(err == 0,  "Unable to write prm: [err %d]", err);

	err = zb_prm_read(&area, &prm_rd);
	zassert_true(err == 0,  "Unable to read prm: [err %d]", err);

	err = memcmp(&prm, &prm_rd, sizeof(struct zb_prm));
	zassert_true(err == 0, "Rd/wr prm differs: [err %d]", err);

}

void test_zb_flash(void)
{
	ztest_test_suite(test_zb_flash,
			 ztest_unit_test(test_zb_get_area),
			 ztest_unit_test(test_zb_cmd),
			 ztest_unit_test(test_zb_prm)
			);

	ztest_run_test_suite(test_zb_flash);

}
