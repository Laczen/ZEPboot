/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(main);

extern void test_zb_flash(void);
extern void test_zb_ec256(void);
extern void test_zb_tlv(void);
extern void test_zb_aes(void);
extern void test_zb_image(void);
extern void test_zb_move(void);

void test_main(void)
{
	test_zb_flash();
	test_zb_ec256();
	test_zb_tlv();
	test_zb_aes();
	test_zb_image();
	test_zb_move();
}
