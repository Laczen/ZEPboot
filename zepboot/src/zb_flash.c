/*
 * Copyright (c) 2019 LaczenJMS.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <crc.h>
#include <flash.h>
#include "../include/zb_flash.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(zbflash);

extern const struct slt_area slot_map[];
extern const unsigned int slot_map_cnt;

size_t zb_flash_align_size(struct device *flash_dev, size_t len)
{
	u8_t write_block_size;

	write_block_size = flash_get_write_block_size(flash_dev);
	if (write_block_size <= 1) {
		return len;
	}
	return (len + (write_block_size - 1)) & ~(write_block_size - 1);
}

off_t zb_flash_align_offset(struct device *flash_dev, off_t offset)
{
	u8_t write_block_size;

	write_block_size = flash_get_write_block_size(flash_dev);
	if (write_block_size <= 1) {
		return offset;
	}
	return offset & ~(write_block_size - 1);
}

int zb_flash_erase(struct device *flash_dev, off_t offset, size_t len)
{
	int rc;

	if (!flash_dev) {
		return -ENXIO;
	}

	LOG_WRN("Erasing [%zd] bytes at [%zx]", len, offset);

	rc = flash_write_protection_set(flash_dev, 0);
	if (rc) {
		/* flash protection set error */
		return rc;
	}
	rc = flash_erase(flash_dev, offset, len);
	if (rc) {
		/* flash erase error */
		return rc;
	}
	(void) flash_write_protection_set(flash_dev, 1);
	return 0;

}

int zb_flash_write(struct device *flash_dev, off_t offset,
		    const void *data, size_t len)
{
	const u8_t *data8 = (const u8_t *)data;
	u8_t buf[ALIGN_BUF_SIZE];
	u8_t write_block_size;
	size_t blen;
	int rc = 0;

	if (!flash_dev) {
		return -ENXIO;
	}
	rc = flash_write_protection_set(flash_dev, 0);
	if (rc) {
		/* flash protection set error */
		return rc;
	}

	write_block_size = flash_get_write_block_size(flash_dev);
	blen = len & ~(write_block_size - 1);
	if (blen > 0) {
		rc = flash_write(flash_dev, offset, data8, blen);
		if (rc) {
			/* flash write error */
			goto end;
		}
		offset += blen;
		data8 += blen;
		len -= blen;
	}
	if (len) {
		memcpy(buf, data8, len);
		(void)memset(buf + len, 0xff, write_block_size - len);
		rc = flash_write(flash_dev, offset, buf, write_block_size);
		if (rc) {
			/* flash write error */
			goto end;
		}

	}
end:
	(void) flash_write_protection_set(flash_dev, 1);
	return rc;
}

int zb_flash_read(struct device *flash_dev, off_t offset,
		   void *data, size_t len)
{
	if (!flash_dev) {
		return -ENXIO;
	}
	return flash_read(flash_dev, offset, data, len);
}

u8_t zb_slt_area_cnt(void)
{
	return slot_map_cnt;
}

int zb_slt_area_get(struct zb_slt_area *area,  u8_t slt_idx)
{
	area->slt0_offset = slot_map[slt_idx].slt0_offset;
	area->slt1_offset = slot_map[slt_idx].slt1_offset;
	area->swpstat_offset = slot_map[slt_idx].swpstat_offset;

	area->slt0_size = slot_map[slt_idx].slt0_size;
	area->slt1_size = slot_map[slt_idx].slt1_size;
	area->swpstat_size = slot_map[slt_idx].swpstat_size;

	area->slt0_fldev = device_get_binding(slot_map[slt_idx].slt0_devname);
	if (!area->slt0_fldev) {
		return -ENXIO;
	}
	area->slt1_fldev = device_get_binding(slot_map[slt_idx].slt1_devname);
	if (!area->slt1_fldev) {
		return -ENXIO;
	}
	area->swpstat_fldev =
		device_get_binding(slot_map[slt_idx].swpstat_devname);
	if (!area->swpstat_fldev) {
		return -ENXIO;
	}
	return 0;
}

bool zb_in_ram(off_t address)
{
	return ((address >= DT_SRAM_BASE_ADDRESS) &
	        (address < (DT_SRAM_BASE_ADDRESS + DT_SRAM_SIZE)));
}

bool zb_in_slt_area(struct zb_slt_area *area, u8_t slt, off_t address)
{

	if (slt == 1) {
		return ((address >= (area->slt1_offset)) &
			(address < (area->slt1_offset + area->slt1_size)));
	}
	else {
		return ((address >= (area->slt0_offset)) &
			(address < (area->slt0_offset + area->slt0_size)));
	}
}

/* crc8 calculation and verification in one routine:
 * to update the crc8: (void) zb_cmd_crc8(cmd)
 * to check the crc8: zb_cmd_crc8(cmd) returns 0 if ok
 * !!!! the routine always updates the cmd->crc8 !!!!
 */
static int zb_cmd_crc8(struct zb_cmd *cmd)
{
	u8_t crc8;

	crc8=cmd->crc8;
	cmd->crc8 = crc8_ccitt(0xff, cmd, offsetof(struct zb_cmd, crc8));
	if (cmd->crc8 == crc8) {
		return 0;
	}
	return 1;
}

struct zb_cmd_loc {
	off_t start;
	off_t end;
	struct device *fl_dev;
};

int zb_get_cmd_loc(struct zb_slt_area *area, struct zb_cmd_loc *loc,
		   const u8_t loc_id)
{
	switch (loc_id) {
		case 0:
			loc->fl_dev = area->slt0_fldev;
			loc->end = area->slt0_offset + area->slt0_size;
			loc->start = loc->end - SECTOR_SIZE;
			break;
		case 1:
			loc->fl_dev = area->slt1_fldev;
			loc->end = area->slt1_offset + area->slt1_size;
			loc->start = loc->end - SECTOR_SIZE;
			break;
		case 2:
			loc->fl_dev = area->swpstat_fldev;
			loc->end = area->swpstat_offset + area->swpstat_size;
			loc->start = area->swpstat_offset;
			break;
		default:
			return -ENOTSUP;
			break;
	}
	return 0;
}

int zb_cmd_loc_erase(struct zb_cmd_loc *loc)
{
	return zb_flash_erase(loc->fl_dev, loc->start, loc->end - loc->start);
}

int zb_erase_swpstat(struct zb_slt_area *area)
{
	struct zb_cmd_loc loc;
	int rc;

	rc = zb_get_cmd_loc(area, &loc, 2);
	if (rc) {
		return rc;
	}
	return zb_cmd_loc_erase(&loc);
}

int zb_erase_slt0end(struct zb_slt_area *area)
{
	struct zb_cmd_loc loc;
	int rc;

	rc = zb_get_cmd_loc(area, &loc, 0);
	if (rc) {
		return rc;
	}
	return zb_cmd_loc_erase(&loc);
}

int zb_erase_slt1end(struct zb_slt_area *area)
{
	struct zb_cmd_loc loc;
	int rc;

	rc = zb_get_cmd_loc(area, &loc, 1);
	if (rc) {
		return rc;
	}
	return zb_cmd_loc_erase(&loc);
}

int zb_cmd_read(struct zb_slt_area *area, struct zb_cmd *cmd, u8_t loc_id)
{
	int rc;
	struct zb_cmd re_cmd;
	struct zb_cmd_loc loc;
	off_t off, end;
	struct device *fl_dev;
	bool found = false;
	u32_t re_cmd_u32;

	rc = zb_get_cmd_loc(area, &loc, loc_id);
	if (rc) {
		return rc;
	}

	fl_dev = loc.fl_dev;
	end = loc.end;
	off = loc.start;

	if (loc_id == 0) {
		off += zb_flash_align_size(loc.fl_dev, sizeof(struct zb_prm));
	}

	while (1) {
		rc= zb_flash_read(fl_dev, off, &re_cmd, sizeof(struct zb_cmd));
		if (rc) {
			break;
		}

		memcpy(&re_cmd_u32,&re_cmd,4);
		if (re_cmd_u32 == EMPTY_U32) {
			if (found) {
				rc = 0;
			} else {
				*cmd = re_cmd;
				rc = -ENOENT;
			}
			break;
		}

		if (!zb_cmd_crc8(&re_cmd)) {
			*cmd = re_cmd;
			found = true;
		}

		off += zb_flash_align_size(fl_dev, sizeof(struct zb_cmd));
		if (off >= end) {
			if (found) {
				rc = 0;
			} else {
				rc = -ENOENT;
			}
			break;
		}
	}

	return rc;
}

int zb_cmd_read_slt0end(struct zb_slt_area *area, struct zb_cmd *cmd)
{
	return zb_cmd_read(area, cmd, 0);
}

int zb_cmd_read_slt1end(struct zb_slt_area *area, struct zb_cmd *cmd)
{
	return zb_cmd_read(area, cmd, 1);
}

int zb_cmd_read_swpstat(struct zb_slt_area *area, struct zb_cmd *cmd)
{
	return zb_cmd_read(area, cmd, 2);
}

int zb_cmd_write(struct zb_slt_area *area, struct zb_cmd *cmd, u8_t loc_id)
{
	int rc;
	struct zb_cmd re_cmd;
	struct zb_cmd_loc loc;
	off_t off, end;
	struct device *fl_dev;
	u32_t re_cmd_u32;

	rc = zb_get_cmd_loc(area, &loc, loc_id);
	if (rc) {
		return rc;
	}

	fl_dev = loc.fl_dev;
	end = loc.end;
	off = loc.start;

	if (loc_id == 0) {
		off += zb_flash_align_size(loc.fl_dev, sizeof(struct zb_prm));
	}

	while (1) {
		rc= zb_flash_read(fl_dev, off, &re_cmd, sizeof(struct zb_cmd));
		if (rc) {
			break;
		}
		memcpy(&re_cmd_u32,&re_cmd,4);
		if (re_cmd_u32 == EMPTY_U32) {
			rc = 0;
			break;
		}

		off += zb_flash_align_size(fl_dev, sizeof(struct zb_cmd));
		if (off >= end) {
			rc = -ENOSPC;
			break;
		}
	}

	if (!rc) {
		(void) zb_cmd_crc8(cmd);
		rc = zb_flash_write(fl_dev, off, cmd, sizeof(struct zb_cmd));
	}

	return rc;
}

int zb_cmd_write_slt0end(struct zb_slt_area *area, struct zb_cmd *cmd)
{
	return zb_cmd_write(area, cmd, 0);
}

int zb_cmd_write_slt1end(struct zb_slt_area *area, struct zb_cmd *cmd)
{
	return zb_cmd_write(area, cmd, 1);
}

int zb_cmd_write_swpstat(struct zb_slt_area *area, struct zb_cmd *cmd)
{
	return zb_cmd_write(area, cmd, 2);
}


int zb_prm_read(struct zb_slt_area *area, struct zb_prm *prm)
{
	int rc;
	off_t off;
	off = area->slt0_offset + area->slt0_size - SECTOR_SIZE;
	rc = zb_flash_read(area->slt0_fldev, off, prm, sizeof(struct zb_prm));
	if (rc) {
		return rc;
	}
	if (prm->pri_ld_address == EMPTY_U32) {
		return -ENOENT;
	}
	return 0;
}

int zb_prm_write(struct zb_slt_area *area, struct zb_prm *prm)
{
	off_t off;
	off = area->slt0_offset + area->slt0_size - SECTOR_SIZE;
	return zb_flash_write(area->slt0_fldev, off, prm,
			      sizeof(struct zb_prm));
}
