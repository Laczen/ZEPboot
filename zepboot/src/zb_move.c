/*
 * Copyright (c) 2019 LaczenJMS.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <errno.h>

#include "../include/zb_move.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(zb_move);

int zb_img_move(zb_move_cmd *mcmd, size_t len, bool to_ram);

int zb_sector_erase(zb_img_info *info, off_t offset)
{
	return zb_flash_erase(info->flash_device, info->hdr_start + offset,
			      SECTOR_SIZE);
}

void set_mcmd_moveup(zb_move_cmd *mcmd, zb_img_swp_info *swp_info,
		     off_t secoff) {
	mcmd->fr_off = swp_info->to.hdr_start + secoff;
	mcmd->fr_eoff = mcmd->fr_off + SECTOR_SIZE;
	mcmd->fl_dev_fr = swp_info->to.flash_device;
	mcmd->to_off = swp_info->to.hdr_start + secoff + SECTOR_SIZE;
	mcmd->fl_dev_to = swp_info->to.flash_device;
}

void set_mcmd_swp_p2(zb_move_cmd *mcmd, zb_img_swp_info *swp_info,
		     off_t secoff) {
	mcmd->fr_off = swp_info->to.hdr_start + secoff + SECTOR_SIZE;
	mcmd->fr_eoff = swp_info->to.enc_start + SECTOR_SIZE;
	mcmd->fl_dev_fr = swp_info->to.flash_device;
	mcmd->to_off = swp_info->fr.hdr_start + secoff;
	mcmd->fl_dev_to = swp_info->fr.flash_device;
	mcmd->key = swp_info->to.enc_key;
}

void set_mcmd_swp_p1(zb_move_cmd *mcmd, zb_img_swp_info *swp_info,
		     off_t secoff) {
	mcmd->fr_off = swp_info->fr.hdr_start + secoff;
	mcmd->fr_eoff = swp_info->fr.enc_start;
	mcmd->fl_dev_fr = swp_info->fr.flash_device;
	mcmd->to_off = swp_info->to.hdr_start + secoff;
	mcmd->fl_dev_to = swp_info->to.flash_device;
	mcmd->key = swp_info->fr.enc_key;
}

int zb_img_cmd_proc_p3_wrt(struct zb_slt_area *area, struct zb_cmd cmd)
{
	u32_t crc32;
	zb_img_info info;
	struct zb_prm prm;

	zb_img_get_info_nsc(&info, area, 0, 0, false);
	zb_img_calc_crc32(&info, &crc32);
	prm.slt0_crc32 = crc32;
	zb_img_conv_version_u32(&info.version, &prm.slt0_ver);
	prm.sec_ld_address = info.load_address;
	prm.pri_ld_address = info.load_address;

	zb_img_get_info_nsc(&info, area, 1, 0, false);
	if (info.is_valid) {
		zb_img_calc_crc32(&info, &crc32);
		prm.slt1_crc32 = crc32;
		zb_img_conv_version_u32(&info.version, &prm.slt1_ver);
		if (cmd.cmd2 & CMD2_MASK_INPLACE) {
			prm.pri_ld_address = info.load_address;
		}
	} else {
		prm.slt1_crc32 = 0xffffffff;
		prm.slt1_ver = 0;
		if (!(cmd.cmd1 & CMD1_MASK_SWP_PERM)) {
			/* disable restore of bad image/no image */
			cmd.cmd1 |= CMD1_MASK_SWP_PERM;
		}
	}
	/* write the information to last sector of slt0 */
	zb_prm_write(area, &prm);
	/* write the executed swap command to slt0end */
	zb_cmd_write_slt0end(area, &cmd);
	return 0;
}

int zb_img_cmd_proc_p4_wrt(struct zb_slt_area *area, struct zb_cmd cmd)
{
	return 0;
}

int zb_get_img_swp_info(zb_img_swp_info *swp_info, struct zb_cmd cmd,
			struct zb_slt_area *area)
{
	u8_t slt0 = 0U, slt1 = 1;
	off_t eoff = 0U;

	LOG_INF("Request image info for move");

	if (((cmd.cmd2 & ~CMD2_MASK_INPLACE) == CMD2_SWP_P1) ||
	    ((cmd.cmd2 & ~CMD2_MASK_INPLACE) == CMD2_SWP_P2)) {
		if (cmd.cmd3 == 0) {
			/* During the header swap the header in the to sector is
		 	 * moved up by one sector */
			eoff = SECTOR_SIZE;
		} else {
			slt0 = 1;
			slt1 = 0U;
		}
	}

	if (cmd.cmd2 & CMD2_MASK_INPLACE) {
		slt0 = 1;
		slt1 = 1;
		eoff = 0U;
	}

	zb_img_get_info_nsc(&swp_info->to, area, slt0, eoff, 0);
	LOG_INF("SWP info to [off %zx start %zx eoff %zx end %zx]",
		swp_info->to.hdr_start, swp_info->to.start,
		swp_info->to.enc_start, swp_info->to.end);
	zb_img_get_info_nsc(&swp_info->fr, area, slt1, eoff, 0);
	LOG_INF("SWP info fr [off %zx start %zx eoff %zx end %zx]",
		swp_info->fr.hdr_start, swp_info->fr.start,
		swp_info->fr.enc_start, swp_info->fr.end);

	if (zb_in_ram(swp_info->fr.load_address)) {
		/* ram load images are not decrypted during swap */
		swp_info->fr.enc_start = swp_info->fr.end;
	}

	if (zb_in_ram(swp_info->to.load_address)) {
		/* ram load images are not decrypted during swap */
		swp_info->to.enc_start = swp_info->to.end;
	}

	swp_info->loaded = true;

	return 0;
}

int zb_img_cmd_proc(zb_img_swp_info *info, struct zb_slt_area *area) {
	int rc;
	struct zb_cmd cmd;
	zb_move_cmd mcmd;
	off_t cmd_off, addr;
	size_t len, end_fr, end_to;
	bool inplace = false;

	while (1) {
		rc = zb_cmd_read_swpstat(area, &cmd);

		if (rc || (cmd.cmd1 == CMD1_ERROR) ||
		    ((cmd.cmd2 & ~CMD2_MASK_INPLACE) < CMD2_SWP_START) ||
		    ((cmd.cmd2 & ~CMD2_MASK_INPLACE) >= CMD2_SWP_END)) {
			break;
		}

		cmd_off = cmd.cmd3 * SECTOR_SIZE;

		if (!info->loaded) {
			rc = zb_get_img_swp_info(info, cmd, area);
			if (rc) {
				LOG_INF("Error in image info");
				/* stop swap */
				cmd.cmd1 = CMD1_ERROR;
			}
		}

		end_fr = info->fr.end - info->fr .hdr_start;
		if (info->to.is_valid) {
			end_to = info->to.end - info->to.hdr_start;
		} else {
			end_to = 0;
		}

		if (cmd.cmd2 & CMD2_MASK_INPLACE) {
			inplace = true;
		}
		cmd.cmd2 &= ~CMD2_MASK_INPLACE;

 		switch (cmd.cmd2) {

			case CMD2_SWP_START: /* start move */
				LOG_INF("Start move");
				cmd.cmd3 = 0;
				if ((inplace) &&
				    (info->fr.enc_start == info->fr.end)) {
					/* no need to do move for unencrypted
					 * images that remain in slot1
					 */
					LOG_INF("No move required");
					cmd.cmd2 = CMD2_SWP_P3;
					break;
				}
				if (end_to == 0) {
					/* no need to do move up when there is
					 * no image in slt0
					 * instead schedule swap
					 */
					LOG_INF("Move up not required");
					cmd.cmd2 = CMD2_SWP_P1;
					break;
				}
				addr = info->to.hdr_start;
				while ((addr + SECTOR_SIZE) < info->to.end) {
					cmd.cmd3++;
					addr += SECTOR_SIZE;
				}
				/* schedule move_up */
				cmd.cmd2 = CMD2_MOVE_UP;
				break;

			case CMD2_MOVE_UP: /* move up to sectors */
				LOG_INF("Move up [sector:%d]", cmd.cmd3);
				/* erase sector cmd.sector+1 */
				zb_sector_erase(&(info->to), cmd_off +
						SECTOR_SIZE);
				/* copy sector cmd.sector to cmd.sector+1 */
				set_mcmd_moveup(&mcmd, info, cmd_off);
				zb_img_move(&mcmd, SECTOR_SIZE, false);
				/* until cmd.sector = 0 */
				if (cmd.cmd3 == 0) {
					if (inplace) {
						cmd.cmd2 = CMD2_SWP_P2;
					} else {
						cmd.cmd2 = CMD2_SWP_P1;
					}
				} else {
					cmd.cmd3 -= 1;
				}
				break;
			/* Swap sectors */
			case CMD2_SWP_P1: /* Move from 1 to 0 */
				if (cmd_off >= end_fr) {
					if (cmd_off >= end_to) {
						cmd.cmd2 = CMD2_SWP_P3;
					} else {
						cmd.cmd2 = CMD2_SWP_P2;
					}
					break;
				}
				LOG_INF("Swap phase 1 [sector:%d]", cmd.cmd3);
				/* erase to sector */
				zb_sector_erase(&(info->to), cmd_off);
				/* copy cmd.sector from fr_slt to to_slt
				 * doing decryption if required
				 */
				set_mcmd_swp_p1(&mcmd, info, cmd_off);
				len = MIN(end_fr - cmd_off, SECTOR_SIZE);
				zb_img_move(&mcmd, len, false);
				cmd.cmd2 = CMD2_SWP_P2;
				break;
			case CMD2_SWP_P2: /* Move from 0 to 1 or 1 to 1 */
				if (cmd_off >= end_to) {
					if (cmd_off >= end_fr) {
						cmd.cmd2 = CMD2_SWP_P3;
					} else {
						cmd.cmd2 = CMD2_SWP_P1;
						cmd.cmd3++;
					}
					break;
				}
				LOG_INF("Swap phase 2 [sector:%d]", cmd.cmd3);
				/* erase fr sector */
				zb_sector_erase(&(info->fr), cmd_off);
				/* copy cmd.sector+1 from to_slt to cmd.sector
				 * in fr_slt doing decryption if required
				 */
				set_mcmd_swp_p2(&mcmd, info, cmd_off);
				len = MIN(end_to - cmd_off, SECTOR_SIZE);
				zb_img_move(&mcmd, len, false);
				cmd.cmd3++;
				if (inplace) {
					cmd.cmd2 = CMD2_SWP_P2;
				} else {
					cmd.cmd2 = CMD2_SWP_P1;
				}
				break;
			case CMD2_SWP_P3:
				LOG_INF("Swap phase 3 [slot0 end]");
				zb_erase_slt0end(area);
				if (inplace) {
					cmd.cmd2 |= CMD2_MASK_INPLACE;
				}
				zb_img_cmd_proc_p3_wrt(area, cmd);
				cmd.cmd2 = CMD2_SWP_P4;
				break;
			case CMD2_SWP_P4:
				LOG_INF("Swap phase 4 [slot1 end]");
				zb_erase_slt1end(area);
				if (inplace) {
					cmd.cmd2 |= CMD2_MASK_INPLACE;
				}
				zb_img_cmd_proc_p4_wrt(area, cmd);
				cmd.cmd2 = CMD2_SWP_END;
				break;
		}
		if (inplace) {
			cmd.cmd2 |= CMD2_MASK_INPLACE;
		}
		rc = zb_cmd_write_swpstat(area, &cmd);
	}

	rc = zb_cmd_read_swpstat(area,&cmd);

	if (cmd.cmd2 & CMD2_MASK_INPLACE) {
		LOG_INF("Finished inplace swap");
	} else {
		LOG_INF("Finished classic swap");
	}

	return 0;
}

int zb_img_swap(struct zb_slt_area *area)
{
	int rc;
	zb_img_swp_info info;
	struct zb_cmd cmd;
	bool swap = false;
	u8_t slt;

	/* Swap is needed if:
	 * a. There was a swap going on
	 * b. A new swap command is given
	 */

	if ((!zb_cmd_read_swpstat(area, &cmd)) &&
	    ((cmd.cmd2 & ~CMD2_MASK_INPLACE) >= CMD2_SWP_START) &&
	    ((cmd.cmd2 & ~CMD2_MASK_INPLACE) <  CMD2_SWP_END)) {
			swap = true;
	}

	if (!swap) {
		/* Handle swap request */
		LOG_INF("Reading slt1");
		rc = zb_cmd_read_slt1end(area, &cmd);
		if ((!rc) && (cmd.cmd1 & CMD1_MASK_SWP_REQUEST)) {
			cmd.cmd1 &= ~CMD1_MASK_SWP_REQUEST;
			if (!zb_img_check(area, &slt)) {
				cmd.cmd2 = CMD2_SWP_START;
				if (slt == 1) {
					cmd.cmd2 |= CMD2_MASK_INPLACE;
					/* inplace images always permanent */
					cmd.cmd1 |= CMD1_MASK_SWP_PERM;
				}
				cmd.cmd3 = 0x0;
				zb_erase_swpstat(area);
				rc = zb_cmd_write_swpstat(area, &cmd);
				if (!rc) {
					swap = true;
				}
			} else {
				LOG_ERR("Bad Image");
			}
		}
	}

	if (!swap) {
		/* Handler request to make permanent */
		rc = zb_cmd_read_slt1end(area, &cmd);
		if ((!rc) && (cmd.cmd1 & CMD1_MASK_SWP_PERM)) {
			rc = zb_cmd_read_slt0end(area, &cmd);
			if ((!rc) && (!(cmd.cmd1 & CMD1_MASK_SWP_PERM))) {
				cmd.cmd1 |= CMD1_MASK_SWP_PERM;
			}
			rc = zb_cmd_write_slt0end(area, &cmd);
		}
	}

	if (!swap) {
		/* Handle temporary swap (restore) */
		rc = zb_cmd_read_slt0end(area, &cmd);
		if ((!rc) && (!(cmd.cmd1 & CMD1_MASK_SWP_PERM))) {
			cmd.cmd1 |= CMD1_MASK_SWP_PERM;
			cmd.cmd2 = CMD2_SWP_START;
			cmd.cmd3 = 0;
			zb_erase_swpstat(area);
			rc = zb_cmd_write_swpstat(area, &cmd);
			if (!rc) {
				swap = true;
			}
		}
	}

	if (swap) {
		info.loaded = false;
		return zb_img_cmd_proc(&info, area);
	}

	return rc;
}



void set_mcmd_ramcopy(zb_move_cmd *mcmd, zb_img_info *info) {
	mcmd->fr_off = info->start;
	mcmd->fr_eoff = info->enc_start;
	mcmd->fl_dev_fr = info->flash_device;
	mcmd->to_off = info->load_address;
}

int zb_img_ram_move(zb_img_info *info)
{
	zb_move_cmd mcmd;

	set_mcmd_ramcopy(&mcmd, info);
	return zb_img_move(&mcmd, info->end - info->start, true);
}

int zb_img_move(zb_move_cmd *mcmd, size_t len, bool to_ram)
{
	u8_t buf[MOVE_BLOCK_SIZE];
	u8_t ctr[AES_BLOCK_SIZE] = {0U};
	size_t ulen = 0; /* unencrypted length */
	off_t fr_off, to_off;
	int j;

	LOG_INF("Sector move: FR [off %zx] [eoff %zx] TO [off %zx]",
		mcmd->fr_off, mcmd->fr_eoff, mcmd->to_off);


	if (mcmd->fr_off < mcmd->fr_eoff) {
		ulen = mcmd->fr_eoff - mcmd->fr_off;
	}

	/* At the moment set the ctr to zero, this could be changed to a proper
	 * nonce if wanted.
	 */
	memset(ctr, 0, AES_BLOCK_SIZE);

	fr_off = mcmd->fr_eoff;
	while ((fr_off < mcmd->fr_off) && (!ulen)) {
		/* ctr increment is required */
		for (j = AES_BLOCK_SIZE; j > 0; --j) {
                	if (++ctr[j - 1] != 0) {
                    		break;
                	}
            	}
		fr_off += AES_BLOCK_SIZE;
	}

	fr_off = mcmd->fr_off;
	to_off = mcmd->to_off;

	while (len) {
		size_t buf_len = MIN(len, MOVE_BLOCK_SIZE);

		if (ulen) {
			buf_len = MIN(buf_len, ulen);
		}

		(void)zb_flash_read(mcmd->fl_dev_fr, fr_off, buf, buf_len);

		if (!ulen) {
			(void)zb_aes_ctr_mode(buf, buf_len, ctr, mcmd->key);
		}

		if (!to_ram) {
			(void)zb_flash_write(mcmd->fl_dev_to, to_off, buf,
					     buf_len);
		} else {
			(void)memcpy((void *)to_off, buf, buf_len);
		}

		if (ulen) {
			ulen -= buf_len;
		}
		len -= buf_len;
		fr_off += buf_len;
		to_off += buf_len;
	}

	return 0;
}