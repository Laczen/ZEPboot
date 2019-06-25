/*
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef H_ZB_MOVE_
#define H_ZB_MOVE_

#include <sys/types.h>
#include "zb_flash.h"
#include "zb_image.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MOVE_BLOCK_SIZE 512

/*
 * Commands are written to flash as a set of 3 bytes (cmd1, cmd2, cmd3)
 * followed by a crc8. cmd1 is used to track general properties: type of swap,
 * error state, ... cmd2 is used to track the steps during the swap process and
 * cmd3 is used to track the sector being processed.
 */

/* cmd1 definitions */
#define CMD1_ERROR		0b10000000
#define CMD1_MASK_SWP_PERM	0b00000001
#define CMD1_MASK_SWP_REQUEST	0b00010000 /* swap request */
#define CMD1_MASK_BT0_REQUEST	0b00100000 /* boot slot 0 request */

/* cmd2 definitions */

#define CMD2_MASK_INPLACE	0b00100000 /* set automatically by load_addr */
#define CMD2_SWP_START		0b00010000
#define CMD2_MOVE_UP		0b00010010
#define CMD2_SWP_P1		0b00010100 /* Phase 1:
				    	    * a. erase to sect x,
					    * b. move fr sect x -> to sect x
					    */
#define CMD2_SWP_P2		0b00010110 /* Phase 2:
					    * a. Erase fr sect x,
					    * b. Move to sect x+1 -> fr sect x
					    */
#define CMD2_SWP_P3		0b00011000 /* Phase 3:
					    * a. Erase last to sector
					    * b. Write images prm / extra info
					    * to last to sector
					    */
#define CMD2_SWP_P4		0b00011001 /* Phase 4:
					    * a. Erase last fr sector
					    * b. Write info to last fr sector
					    */
#define CMD2_SWP_END		0b00011111

/**
 * @brief zb_move_cmd: necessary info to do a move of a sector or a move from
 * flash to ram
 * @{
 */

typedef struct {
	off_t fr_off;	/* offset of sector that is moved */
	off_t fr_eoff;  /* offset from where encryption starts, this is
			   used to calculate the nonce and to allow part
			   of a sector to be moved without encryption
			   for RAM load this is also used to define the
			   image start */
	off_t to_off;	/* offset of sector to move to */
	u8_t *key;	/* pointer to encryption key */
	struct device *fl_dev_fr;
	struct device *fl_dev_to;
} zb_move_cmd;

/**
 * @}
 */

/**
 * @brief zb_img_swap_info: necessary info to do a complete swap
 * @{
 */

typedef struct {
	zb_img_info to;	/* information about image in the to area */
	zb_img_info fr;	/* information about image in the from area */
	bool loaded;	/* has the information been loaded ? */
} zb_img_swp_info;

/**
 * @}
 */

/**
 * @brief zb_move API
 * @{
 */

/**
 * @brief zb_img_swap
 *
 * Continues or starts swapping of images in specified area
 *
 * @param[in] area Pointer to zb_slt_area that contains the images to be swapped
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_img_swap(struct zb_slt_area *area);

/**
 * @brief zb_img_ram_move
 *
 * Continues or starts swapping of images in specified area
 *
 * @param[in] info Pointer to zb_img_info that contains the required info about
 * the image to be moved to RAM
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_img_ram_move(zb_img_info *info);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
