/*
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 #ifndef H_ZB_IMAGE_
 #define H_ZB_IMAGE_

#include <sys/types.h>
#include <device.h>
#include "zb_flash.h"
#include "zb_aes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TLVE type 0x00 cannot be used, reserved for internal usage !! */

#define TLVE_IMAGE_TYPE 0x10
#define TLVE_IMAGE_TYPE_BYTES 1

typedef struct __packed {
    u8_t    major;
    u8_t    minor;
    u16_t   revision;
    u32_t   build;
} img_ver;

typedef struct __packed {
    off_t   start; /* offset from the tlv area start */
    size_t  size;
    off_t   load_address;
    img_ver version;
} zb_tlv_img_info;

#define TLVE_IMAGE_INFO 0x20
#define TLVE_IMAGE_INFO_BYTES sizeof(zb_tlv_img_info)

#define TLVE_IMAGE_HASH 0x30
#define TLVE_IMAGE_HASH_BYTES HASH_BYTES

#define TLVE_IMAGE_EPUBKEY 0x40
#define TLVE_IMAGE_EPUBKEY_BYTES PUBLIC_KEY_BYTES

/** @brief image API structures
 * @{
 */

/**
 * @brief img_info: contains all the info required during a move
 */
typedef struct {
    off_t hdr_start;
    off_t start;
    off_t enc_start;
    off_t end;
    u32_t load_address;
    img_ver version;
    u8_t enc_key[AES_BLOCK_SIZE];
    u8_t type;
    struct device *flash_device;
    bool is_valid;
} zb_img_info;

/**
 * @}
 */

/**
 * @brief image API
 * @{
 */

/**
 * @brief zb_img_get_info_nsc
 *
 * reads the image info without signature check (nsc: no signature check)
 *
 * @param img_info pointer to store info in, if it is valid image info the
 *                 img_info.is_valid flag is set
 * @param slt_idx slot_area index
 * @param slt slot 0 or 1
 * @param eoff extra offset (used in case the image is in a shifted position)
 * @param val_img validate image (check hash)
 */
void zb_img_get_info_nsc(zb_img_info *info, struct zb_slt_area *area, u8_t slt,
                         off_t eoff, bool val_img);

/**
 * @brief zb_img_get_info_wsc
 *
 * reads the image info with signature check (wsc: with signature check)
 *
 * @param img_info pointer to store info in, if it is valid image info the
 *                 img_info.is_valid flag is set
 * @param slt_idx slot_area index
 * @param slt slot 0 or 1
 * @param eoff extra offset (used in case the image is in a shifted position)
 * @param val_img validate image (check hash)
 */
void zb_img_get_info_wsc(zb_img_info *info, struct zb_slt_area *area, u8_t slt,
                         off_t eoff, bool val_img);

/**
 * @brief zb_img_calc_crc32
 *
 * calculates the crc32 over the image
 *
 * @param img_info image info this needs to be set first
 * @param crc32 calculated crc
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_img_calc_crc32(zb_img_info *info, u32_t *crc32);

/**
 * @brief zb_img_conv_version_u32
 *
 * convert image version to u32 format, ignore build number
 *
 * @param[in] version read from image info
 * @param[out] u32 format of version
 */
void zb_img_conv_version_u32(img_ver *ver, u32_t *u32_ver);

/**
 * @brief zb_img_check
 *
 * check validity of image in slt1
 *
 * @param[in] area to check
 * @param[out] slt where to move the image (slt0 or slt1)
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_img_check(struct zb_slt_area *area, u8_t *slt);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
