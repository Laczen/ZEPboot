/*
 * Copyright (c) 2019 LaczenJMS.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <errno.h>
#include <crc.h>
#include <tinycrypt/ecc_dh.h>
#include <tinycrypt/ecc_dsa.h>
#include <tinycrypt/sha256.h>
#include "../include/zb_flash.h"
#include "../include/zb_ec256.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(zb_ec256);

extern const char ec256_boot_pri_key[];
extern const unsigned int ec256_boot_pri_key_len;
extern const char ec256_root_pub_key[];
extern const unsigned int ec256_root_pub_key_len;


int zb_get_encr_key(u8_t *key, const u8_t *pubkey, u8_t keysize)
{
	int rc;
	u8_t ext[4] = {0};
	u8_t secret[SHARED_SECRET_BYTES] = {0};
	u8_t digest[HASH_BYTES] = {0};

	const struct uECC_Curve_t * curve = uECC_secp256r1();
	struct tc_sha256_state_struct s;

	if (keysize > PRIVATE_KEY_BYTES) {
		return -EFAULT;
	}
	if (uECC_valid_public_key(pubkey, curve) != 0) {
		return -EFAULT;
	};

	rc = uECC_shared_secret(pubkey, ec256_boot_pri_key, secret, curve);
	if (!rc) {
		return -EFAULT;
	}

	rc = tc_sha256_init(&s);
	if (!rc) {
		return -EFAULT;
	}

	rc = tc_sha256_update(&s, secret, SHARED_SECRET_BYTES);
	if (!rc) {
		return -EFAULT;
	}

	rc = tc_sha256_update(&s, ext, 4);
	if (!rc) {
		return -EFAULT;
	}

	rc = tc_sha256_final(digest, &s);
	if (!rc) {
		return -EFAULT;
	}
	memcpy(key, digest, keysize);

	memset(digest, 0, HASH_BYTES);
	memset(secret, 0, SHARED_SECRET_BYTES);

	return 0;
}

int zb_sign_verify(const u8_t *hash, const u8_t *signature)
{
	int cnt;
	u8_t pubk[PUBLIC_KEY_BYTES];
	const struct uECC_Curve_t * curve = uECC_secp256r1();

	/* validate the hash for each of the root pubkeys */
	cnt = 0;
	while (cnt < ec256_root_pub_key_len) {
		memcpy(pubk, &ec256_root_pub_key[cnt], PUBLIC_KEY_BYTES);
		cnt += PUBLIC_KEY_BYTES;
		if (uECC_valid_public_key(pubk, curve) != 0) {
			continue;
		}
		if (uECC_verify(pubk, hash, VERIFY_BYTES, signature, curve)) {
			return 0;
		}
	}
	return -EFAULT;
}

int zb_hash_flash(u8_t *hash, struct device *fl_dev, off_t off, size_t len)
{
	int rc;
	struct tc_sha256_state_struct s;
	u8_t buf[HASH_FLASH_BUFFER_BYTES]={0};
	off_t start;
	size_t jump;

	rc = tc_sha256_init(&s);
	if (!rc) {
		return -EFAULT;
	}

	start = zb_flash_align_offset(fl_dev, off);
	jump = off - start;
	len += jump;
	while (len > 0) {
		size_t buf_len = MIN(HASH_FLASH_BUFFER_BYTES, len);
		rc = zb_flash_read(fl_dev, start, &buf, buf_len);
		if (rc) {
			goto end;
		}

		rc = tc_sha256_update(&s, buf + jump, buf_len - jump);
		if (!rc) {
			rc = -EFAULT;
			goto end;
		}
		start += buf_len;
		len -= buf_len;
		jump = 0;
	}

	rc = tc_sha256_final(hash, &s);
	if (rc) {
		return 0;
	}
	return -EFAULT;
end:
	return rc;
}

int zb_crc32_flash(u32_t *crc32, struct device *fl_dev, off_t off, size_t len)
{
	int rc;
	u8_t buf[HASH_FLASH_BUFFER_BYTES]={0};
	off_t start;
	size_t jump;
	u32_t crc = 0;

	start = zb_flash_align_offset(fl_dev, off);
	jump = off - start;
	len += jump;
	while (len > 0) {
		size_t buf_len = MIN(HASH_FLASH_BUFFER_BYTES, len);

		rc = zb_flash_read(fl_dev, start, &buf, buf_len);
		if (rc) {
			goto end;
		}

		crc = crc32_ieee_update(crc, buf + jump, buf_len - jump);

		start += buf_len;
		len -= buf_len;
		jump = 0;
	}

	*crc32 = crc;
	return 0;
end:
	return rc;
}