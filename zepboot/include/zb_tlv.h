/*
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 #ifndef H_ZB_TLV_
 #define H_ZB_TLV_

#include <sys/types.h>
#include "zb_ec256.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TLV_AREA_MAGIC 0x544c5641 /* TLVA in hex */
#define TLV_AREA_MIN_SIZE 256
#define TLV_AREA_MAX_SIZE 1024

#define TLV_AREA_SIGN_SIZE SIGNATURE_BYTES

/* tlv (type length value) area header definition:
 *
 * A tlv area is defined as a header and a set of tlv entries.
 *
 * Each tlv entry consists of a type (u8_t), a length (u8_t) and a value (*u8_t)
 *
 * The tlv area header contains a magic value, the tlv size (including header)
 * a type, a signature type and the signature. The signature is generated from * a hash value calculated over all tlv entries, but excluding the tlv header.
 */
typedef struct __packed {
    u32_t tlva_magic;
    u16_t tlva_size; /* total size of tlv area including header */
    u8_t tlva_type;
    u8_t tlva_sig_type;
    u8_t tlva_signature[SIGNATURE_BYTES];
} tlv_area_hdr;

/* In a tlv area a entry has a type, a length and a value */
/* A entry type of 0x00 is reserved for internal usage */

typedef struct {
    u8_t type;
    u8_t length;
    u8_t *value;
} tlv_entry;

/**
 * @brief tlv API
 * @{
 */

/**
 * @brief zb_open_tlv_area
 *
 * opens the tlv area, validates the tlv area if requested.
 *
 * @param fldev: flash device where the area is located
 * @param offset: offset where the area is located
 * @param data: return buffer
 * @param validate: if set to yes will validate the tlv area before returning it
 * @retval -ERRNO errno code if error
 * @retval size of the tlv area excluding the header and signature
 */
int zb_open_tlv_area(struct device *flash_dev, off_t offset, void *data,
		     bool validate);

/**
 * @brief zb_step_tlv
 *
 * step through the tlv.
 *
 * @param data: pointer to ram where to search for tlv
 * @param offset: offset in data to start read (updated by the routine)
 * @param entry: return buffer
 * @retval None
 */
void zb_step_tlv(const void *data, off_t *offset, tlv_entry *entry);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
