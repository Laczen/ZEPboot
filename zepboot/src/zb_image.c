/*
 * Copyright (c) 2019 LaczenJMS.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <crc.h>

#include "../include/zb_flash.h"
#include "../include/zb_ec256.h"
#include "../include/zb_tlv.h"
#include "../include/zb_image.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(zb_image);

int zb_img_get_info(zb_img_info *info, struct zb_slt_area *area, u8_t slt,
		    off_t eoff, bool val_tlv,  bool val_img)
{
	int rc, tlv_size;;
	off_t offset;
	tlv_entry entry;
	zb_tlv_img_info rd_info;
	u8_t tlv[TLV_AREA_MAX_SIZE];
	u8_t calc_hash[HASH_BYTES];
	struct device *fl_dev;

	info->is_valid = false;
	offset = eoff;
	if (slt == 1) {
		offset += area->slt1_offset;
		fl_dev = area->slt1_fldev;

	} else {
		offset += area->slt0_offset;
		fl_dev = area->slt0_fldev;
	}

	info->flash_device = fl_dev;
	info->hdr_start = offset - eoff;
	info->start = info->hdr_start;
	info->enc_start = info->hdr_start;
	info->end = info->hdr_start;
	info->load_address = info->hdr_start;
	memset(&(info->version), 0, sizeof(img_ver));

	/* open the tlv area, only do signature verification for slt1 */
	tlv_size = zb_open_tlv_area(fl_dev, offset, tlv, val_tlv);

	if (tlv_size < 0) {
		return tlv_size;
	}

	offset = 0;
	entry.type = 0;
	while ((entry.type != TLVE_IMAGE_TYPE) && (offset < tlv_size)) {
		zb_step_tlv(tlv, &offset, &entry);
	}
	if ((entry.type != TLVE_IMAGE_TYPE) ||
	    (entry.length != TLVE_IMAGE_TYPE_BYTES)) {
		return -EFAULT;
	}
	memcpy(&info->type, entry.value, entry.length);

	offset = 0;
	entry.type = 0;
	while ((entry.type != TLVE_IMAGE_INFO) && (offset < tlv_size)) {
		zb_step_tlv(tlv, &offset, &entry);
	}
	if ((entry.type != TLVE_IMAGE_INFO) ||
	    (entry.length != TLVE_IMAGE_INFO_BYTES)) {
		return -EFAULT;
	}
	memcpy(&rd_info, entry.value, entry.length);

	info->start = info->hdr_start + rd_info.start;
	info->end = info->start + rd_info.size;
	info->load_address = rd_info.load_address;
	info->version = rd_info.version;

	offset = 0;
	entry.type = 0;
	while ((entry.type != TLVE_IMAGE_HASH) && (offset < tlv_size)) {
		zb_step_tlv(tlv, &offset, &entry);
	}
	if ((entry.type != TLVE_IMAGE_HASH) ||
	    (entry.length != TLVE_IMAGE_HASH_BYTES)) {
		return -EFAULT;
	}
	if (val_img) {
		rc = zb_hash_flash(calc_hash, fl_dev, info->start + eoff,
				   rd_info.size);
		if (memcmp(entry.value, calc_hash, HASH_BYTES)) {
			return -EFAULT;
		}
	}

	offset = 0;
	entry.type = 0;
	while ((entry.type != TLVE_IMAGE_EPUBKEY) && (offset < tlv_size)) {
		zb_step_tlv(tlv, &offset, &entry);
	}
	if ((entry.type != TLVE_IMAGE_EPUBKEY) ||
	    (entry.length != TLVE_IMAGE_EPUBKEY_BYTES)) {
		info->enc_start = info->end;
	} else {
		if (zb_get_encr_key(info->enc_key, entry.value,
				    AES_BLOCK_SIZE)) {
			return -EFAULT;
		}
		info->enc_start = info->start;
	}
	info->is_valid = true;
	return 0;
}

void zb_img_get_info_nsc(zb_img_info *info, struct zb_slt_area *area, u8_t slt,
			 off_t eoff, bool val_img)
{
	zb_img_get_info(info, area, slt, eoff, false, val_img);
}

void zb_img_get_info_wsc(zb_img_info *info, struct zb_slt_area *area, u8_t slt,
			 off_t eoff, bool val_img)
{
	zb_img_get_info(info, area, slt, eoff, true, val_img);
}

int zb_img_calc_crc32(zb_img_info *info, u32_t *crc32)
{
	return zb_crc32_flash(crc32, info->flash_device, info->start,
			      info->end - info->start);
}

void zb_img_conv_version_u32(img_ver *ver, u32_t *u32_ver)
{
	*u32_ver = (((u32_t)ver->major << 24) | ((u32_t)ver->minor << 16) |
		    ((u32_t)ver->revision));
}

int zb_img_check(struct zb_slt_area *area, u8_t *slt)
{
	int rc = 0;
	zb_img_info info;
	size_t img_size;
	struct zb_prm prm;
	u32_t img_version;

	zb_img_get_info_wsc(&info, area, 1, 0, true);
	if (!info.is_valid) {
		return -EFAULT;
	}

	if (zb_in_slt_area(area, 1, info.load_address)) {
		*slt = 1;
	} else {
		*slt = 0;
	}

	img_size = info.end - info.hdr_start;

	if ((*slt == 0) && ((img_size + SECTOR_SIZE) > area->slt0_size)) {
		return -EFAULT;
	}
	if ((*slt == 1) && ((img_size + SECTOR_SIZE) > area->slt1_size)) {
		return -EFAULT;
	}

	rc = zb_prm_read(area, &prm);
	if (rc == -ENOENT) {
		return 0;
	}
	if (rc) {
		return -EFAULT;
	}

	zb_img_conv_version_u32(&info.version, &img_version);

	if ((*slt == 0) && (img_version < prm.slt0_ver)) {
		return -EFAULT;
	}
	if ((*slt == 1) && (img_version < prm.slt1_ver)) {
		return -EFAULT;
	}

	return 0;
}