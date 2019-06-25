/*
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 #ifndef H_ZB_EC256_
 #define H_ZB_EC256_

#include <sys/types.h>
#include <tinycrypt/ecc.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SIGNATURE_BYTES		2 * NUM_ECC_BYTES
#define PUBLIC_KEY_BYTES	2 * NUM_ECC_BYTES
#define PRIVATE_KEY_BYTES	NUM_ECC_BYTES
#define SHARED_SECRET_BYTES	NUM_ECC_BYTES
#define VERIFY_BYTES		NUM_ECC_BYTES
#define HASH_BYTES 		NUM_ECC_BYTES
#define HASH_FLASH_BUFFER_BYTES	256

/**
 * @brief EC256 API
 * @{
 */

/**
 * @brief zb_get_encr_key
 *
 * Set the encryption key using a ec_dh 256 key exchange. The key is derived
 * from the shared secret using a key derivation function (i.e. KDF1).
 * This routine uses the bootloader private key.
 *
 * @param key: returned encryption key
 * @param pubkey: public key used to generate the shared secret
 * @param keysize: expected size of returned encryption key (i.e AES block size)
 * @retval -ERRNO errno code if error
 * @retval 0 if succesfull
 */
int zb_get_encr_key(u8_t *key, const u8_t *pubkey, u8_t keysize);

/**
 * @brief zb_sign_verify
 *
 * Verifies the signature given the hash for a ec_dsa 256 signing.
 * This routine uses the public root keys that are stored in the bootloader.
 *
 * @param hash: calculated message hash
 * @param signature: message hash signature
 * @retval -ERRNO errno code if error
 * @retval 0 if succesfull
 */
int zb_sign_verify(const u8_t *hash, const u8_t *signature);

/**
 * @brief hash_flash
 *
 * Calculates the hash (SHA256) over a region in flash.
 *
 * @param hash: calculated message hash
 * @param flash_dev: flash device of the region
 * @param off: offset of region in flash
 * @param len: region length
 * @retval -ERRNO errno code if error
 * @retval 0 if succesfull
 */
int zb_hash_flash(u8_t *hash, struct device *fl_dev, off_t off, size_t len);

/**
 * @brief crc32_flash
 *
 * Calculates the crc32 over a region in flash.
 *
 * @param crc32: calculated crc
 * @param flash_dev: flash device of the region
 * @param off: offset of region in flash
 * @param len: region length
 * @retval -ERRNO errno code if error
 * @retval 0 if succesfull
 */
int zb_crc32_flash(u32_t *crc32, struct device *fl_dev, off_t off, size_t len);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
