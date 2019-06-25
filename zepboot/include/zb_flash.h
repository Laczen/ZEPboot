/*
 * Flash routines for zepboot:
 * 	a. general purpose flash read/write/erase routines
 * 	b. routines to use image command (cmd) info
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 #ifndef H_ZB_FLASH_
 #define H_ZB_FLASH_

#include <sys/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SECTOR_SIZE DT_FLASH_ERASE_BLOCK_SIZE
#define EMPTY_U8 0xff
#define EMPTY_U32 0xffffffff
#define ALIGN_BUF_SIZE	16

BUILD_ASSERT_MSG(DT_FLASH_WRITE_BLOCK_SIZE < ALIGN_BUF_SIZE,
                 "Unsupported write block size");
/**
 * @brief zb_flash API: general flash routines
 */

size_t zb_flash_align_size(struct device *flash_dev, size_t len);
off_t zb_flash_align_offset(struct device *flash_dev, off_t offset);
int zb_flash_erase(struct device *flash_dev, off_t offset, size_t len);
int zb_flash_write(struct device *flash_dev, off_t offset,
		   const void *data, size_t len);
int zb_flash_read(struct device *flash_dev, off_t offset,
		  void *data, size_t len);

/**
 * @}
 */

/**
 * @brief zb_is_ram: check if address belongs to ram
 * @param address to check
 * @retval true if address is in ram region
 */
bool zb_in_ram(off_t address);
/**
 * @}
 */

/**
 * @brief slot_area: flash is separated in multiple slot areas, each of these
 * slt_area is divided into a slot 0, slot 1 and a swap status area.
 * Images are placed into slot 1 and can then swapped/decrypted into slot 0, or
 * they can remain in slot 1 when they are to be executed from slot 1. In the
 * case they are to be executed in slot 1 they are also decrypted into slot 1.
 *
 * The swap status area is used to keep track of the swap and/or decryption
 * status by writing commands to it.
 *
 * The last sector of slot0 (slt0end) is used during the swap process to
 * temporarily store data of the image in slot0. After the swap is finished it
 * is used to store parameters of the images in slot0 and slot1 (crc32 over the
 * image, version and load address of both images). After the parameters
 * commands can be written to keep track of the boot count for test images.
 *
 * The last sector of slot1 (slt1end) is used during the swap process to
 * temporarily store data of the image in slot1 (only in the case of an inplace
 * decryption). After the swap is finished it is used to write commands to
 * communicate with the bootloader (e.g. start a swap, override the boot count
 * limit, ...)
 */

struct slt_area {
    off_t  slt0_offset;
    off_t  slt1_offset;
    off_t  swpstat_offset;
    size_t slt0_size;
    size_t slt1_size;
    size_t swpstat_size;
    const char *slt0_devname;
    const char *slt1_devname;
    const char *swpstat_devname;
};

struct zb_slt_area {
    off_t  slt0_offset;
    off_t  slt1_offset;
    off_t  swpstat_offset;
    size_t slt0_size;
    size_t slt1_size;
    size_t swpstat_size;
    struct device *slt0_fldev;
    struct device *slt1_fldev;
    struct device *swpstat_fldev;
};

/**
 * @}
 */

/**
 * @brief zb_slt_area API
 * @{
 */

/**
 * @brief zb_slt_area_cnt
 *
 * Returns the number of slt_areas in slot_map.
 *
 * @retval cnt number of slot areas
 */
u8_t zb_slt_area_cnt(void);

/**
 * @brief zb_slt_area_get
 *
 * Returns the slt_area in flash for slot_map[slt_idx].
 *
 * @param fs Pointer to zb_slt_area
 * @param slt_idx Index to slot_area: slot_map[slt_idx].
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_slt_area_get(struct zb_slt_area *area, u8_t slt_idx);

/**
 * @brief zb_in_slt_area
 *
 * Checks if address belongs to area, slt.
 *
 * @param fs Pointer to zb_slt_area
 * @param slt 0 or 1
 * @param address to check
 * @retval true is address belongs to slt_idx of area
 */
bool zb_in_slt_area(struct zb_slt_area *area, u8_t slt, off_t address);

/**
 * @brief zb_erase_swpstat
 *
 * Erases the swpstat area in zb_slt_area.
 *
 * @param fs Pointer to zb_slt_area
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_erase_swpstat(struct zb_slt_area *area);

/**
 * @brief zb_erase_slt0end
 *
 * Erases the slt0end area in zb_slt_area.
 *
 * @param fs Pointer to zb_slt_area
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_erase_slt0end(struct zb_slt_area *area);

/**
 * @brief zb_erase_slt1end
 *
 * Erases the slt1end area in zb_slt_area.
 *
 * @param fs Pointer to zb_slt_area
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_erase_slt1end(struct zb_slt_area *area);

/**
 * @}
 */

/**
 * @brief zb_cmd: command stored in flash
 * @{
 */

struct zb_cmd {
	/*@{*/
	u8_t cmd1;	    /**< command */
    u8_t cmd2;      /**< extended command */
	u8_t cmd3;    /**< sector to process */
	u8_t crc8;	    /**< crc8 calculated over cmd and sector */
	/*@}*/
} __packed;

/**
 * @}
 */

/**
 * @brief zb_cmd API
 * @{
 */

/**
 * @brief zb_cmd_read_swpstat
 *
 * reads last valid cmd from swpstat_area
 *
 * @param area Pointer to zb_slt_area
 * @param cmd Pointer to command
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_cmd_read_swpstat(struct zb_slt_area *area, struct zb_cmd *cmd);

/**
 * @brief zb_cmd_read_slt0end
 *
 * reads last valid cmd from sector at the end of slt0
 *
 * @param area Pointer to zb_slt_area
 * @param cmd Pointer to command
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_cmd_read_slt0end(struct zb_slt_area *area, struct zb_cmd *cmd);

/**
 * @brief zb_cmd_read_slt1end
 *
 * reads last valid cmd from sector at the end of slt1
 *
 * @param area Pointer to zb_slt_area
 * @param cmd Pointer to command
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_cmd_read_slt1end(struct zb_slt_area *area, struct zb_cmd *cmd);

/**
 * @brief zb_cmd_write_swpstat
 *
 * writes new cmd to swpstat_area
 *
 * @param fs Pointer to zb_slt_area
 * @param cmd Pointer to command
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_cmd_write_swpstat(struct zb_slt_area *area, struct zb_cmd *cmd);

/**
 * @brief zb_cmd_write_slt0end
 *
 * writes new cmd to sector at the end of slt0
 *
 * @param fs Pointer to zb_slt_area
 * @param cmd Pointer to command
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_cmd_write_slt0end(struct zb_slt_area *area, struct zb_cmd *cmd);

/**
 * @brief zb_cmd_write_slt1ext
 *
 * writes new cmd to sector at the end of slt1
 *
 * @param fs Pointer to zb_slt_area
 * @param cmd Pointer to command
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_cmd_write_slt1end(struct zb_slt_area *area, struct zb_cmd *cmd);

/**
 * @}
 */

/**
 * @brief zb_prm: parameters describing the images that are in the slot_area
 * @{
 */

struct zb_prm {
	/*@{*/
	off_t pri_ld_address; /**< primary load address */
    off_t sec_ld_address; /**< secondary load address */
    u32_t slt0_crc32; /**< crc32 calculated over signature in slt0 */
	u32_t slt1_crc32; /**< crc32 calculated over signature in slt1 */
    u32_t slt0_ver; /**< version of image in slt0 */
    u32_t slt1_ver; /**< version of image in slt1 */

	/*@}*/
} __packed;

/**
 * @}
 */

/**
 * @brief zb_prm API
 * @{
 */

/**
 * @brief zb_prm_read
 *
 * reads prm from slt0end_area
 *
 * @param area Pointer to zb_slt_area
 * @param prm Pointer to parameters (prm)
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_prm_read(struct zb_slt_area *area, struct zb_prm *prm);

/**
 * @brief zb_prm_write
 *
 * writes prm to slt0end_area
 *
 * @param fs Pointer to zb_slt_area
 * @param cmd Pointer to parameters (prm)
 * @retval 0 Success
 * @retval -ERRNO errno code if error
 */
int zb_prm_write(struct zb_slt_area *area, struct zb_prm *prm);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
