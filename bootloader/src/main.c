/*
 * Copyright (c) 2019 LaczenJMS.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/reboot.h>
#include <device.h>
#include <string.h>
#include <soc.h>
#include "../../zepboot/include/zb_flash.h"
#include "../../zepboot/include/zb_move.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(main);

struct arm_vector_table {
    u32_t msp;
    u32_t reset;
};

static void do_boot(off_t load_address)
{
    struct arm_vector_table *vt;

    vt = (struct arm_vector_table *)(load_address);

    irq_lock();

    __set_MSP(vt->msp);
    ((void (*)(void))vt->reset)();
}

void main(void)
{
	int rc = 0, cnt;
	struct zb_slt_area area;
	struct zb_prm prm;
	struct zb_cmd cmd;
	zb_img_info info;
	u32_t crc32;

	cnt = zb_slt_area_cnt();

	/* Start or continue swap */
	while ((cnt--) > 0) {
		rc = zb_slt_area_get(&area, cnt);
		if (!rc) {
			rc = zb_img_swap(&area);
		}
	}

	/* Boot is done for images slot_map[0], area is already set OK */
	if (!rc) {
		rc = zb_prm_read(&area, &prm);
	}

	if (!rc) {
		zb_cmd_read_slt1end(&area, &cmd);
		if (cmd.cmd1 == CMD1_MASK_BT0_REQUEST) {
			prm.pri_ld_address = prm.sec_ld_address;
		}
	}

	/* Verify crc32 prior to boot */
	if ((!rc) && zb_in_slt_area(&area, 1, prm.pri_ld_address)) {
		zb_img_get_info_nsc(&info, &area, 1, 0, false);
		zb_img_calc_crc32(&info, &crc32);
		if (prm.slt1_crc32 != crc32) {
			prm.pri_ld_address = prm.sec_ld_address;
		}
	}

	if ((!rc) && (zb_in_slt_area(&area, 0, prm.pri_ld_address) ||
		      zb_in_ram(prm.pri_ld_address))) {
		zb_img_get_info_nsc(&info, &area, 0, 0, false);
		zb_img_calc_crc32(&info, &crc32);
		if (prm.slt0_crc32 != crc32) {
			rc = -EFAULT;
		}
		if ((!rc) && zb_in_ram(prm.pri_ld_address)) {
			rc = zb_img_ram_move(&info);
		}
	}

	if (!rc) {
		LOG_INF("Ready to boot [addr %x]", prm.pri_ld_address);
		/* boot */
		do_boot(prm.pri_ld_address);
	} else {
		LOG_ERR("Nothing valid to boot");
	}

}

