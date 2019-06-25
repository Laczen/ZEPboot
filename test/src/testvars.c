/*
 * Copyright (c) 2018 Laczen: Set of keys to test signing and encryption
 * key exchange.
 *
 * Keys definition: the bootloader has two types of keys:
 * a. A private encryption key that is used to derive the encryption key
 * b. A set of root public keys that is used to validate images
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include "../../zepboot/include/zb_ec256.h"

u8_t test_msg[HASH_BYTES] = {
	0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8,
	0xf7, 0xf6, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0,
	0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8,
	0xf7, 0xf6, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0
};


u8_t test_msg_hash[HASH_BYTES] = {
	87, 43, 26, 198, 187, 148, 71, 79,
	220, 38, 223, 89, 4, 118, 115, 109,
	231, 185, 12, 231, 123, 212, 221, 103,
	117, 243, 241, 37, 134, 175, 136, 251
};

u32_t test_msg_crc32 = 0x4D299FC3;

u8_t test_signature[SIGNATURE_BYTES] = {
	252, 86, 17, 53, 85, 142, 204, 211,
	55, 184, 117, 91, 159, 134, 6, 189,
	152, 48, 212, 168, 153, 194, 37, 69,
	159, 214, 44, 58, 39, 64, 19, 36,
	175, 12, 24, 179, 83, 243, 204, 68,
	125, 110, 101, 58, 175, 226, 88, 44,
	44, 100, 215, 166, 176, 109, 107, 238,
	238, 129, 253, 12, 66, 182, 43, 27
};

u8_t test_enc_pub_key[PUBLIC_KEY_BYTES] = {
	162, 232, 188, 223, 146, 244, 15, 182,
	171, 144, 101, 158, 237, 57, 205, 66,
	164, 144, 209, 239, 44, 138, 85, 163,
	97, 222, 197, 80, 96, 83, 34, 117,
	201, 36, 44, 25, 158, 7, 5, 165,
	73, 27, 116, 124, 239, 11, 239, 8,
	54, 48, 235, 178, 121, 206, 207, 21,
	192, 36, 184, 152, 219, 62, 106, 175
};

u8_t test_enc_key[16] = {
	100, 94, 130, 145, 92, 49, 78, 162,
	75, 150, 19, 98, 20, 112, 197, 170
};