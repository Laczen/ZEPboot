/*
 * Copyright (c) 2019 LaczenJMS.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include "../include/zb_flash.h"
#include "../include/zb_tlv.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(zb_tlv);

int zb_open_tlv_area(struct device *flash_dev, off_t offset, void *data,
		     bool validate)
{
	int rc;
	tlv_area_hdr hdr;
	u8_t hash[HASH_BYTES];
	off_t data_off;
	size_t tlv_size = 0;

	rc = zb_flash_read(flash_dev, offset, &hdr, sizeof(tlv_area_hdr));
	if (rc) {
		return rc;
	}

	if (hdr.tlva_magic != TLV_AREA_MAGIC) {
		return -EFAULT;
	}

	data_off = offset + sizeof(tlv_area_hdr);
	tlv_size = (size_t)hdr.tlva_size - sizeof(tlv_area_hdr);

	if (validate) {
		rc = zb_hash_flash(hash, flash_dev, data_off, tlv_size);
		if (rc) {
			return rc;
		}

		rc = zb_sign_verify(hash, hdr.tlva_signature);
		if (rc) {
			return -EFAULT;
		}
	}

	rc = zb_flash_read(flash_dev, data_off, data, tlv_size);
	if (rc) {
	 	return rc;
	}

	return (int)tlv_size;
}

void zb_step_tlv(const void *data, off_t *offset, tlv_entry *entry)
{
    	u8_t *p = (u8_t *)data;

    	p += *offset;

    	entry->type = *p++;
    	entry->length = *p;
	entry->value = p + 1;

	*offset += entry->length + 2;
}