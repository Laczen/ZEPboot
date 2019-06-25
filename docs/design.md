<!--
  Copyright (c) 2018 Laczen

  SPDX-License-Identifier: Apache-2.0
-->

# Bootloader

## Summary

ZEPboot is a zephyr based bootloader that supports booting and upgrading of
signed and (optionally) encrypted images. ZEPboot's functionality is limited
to moving (valid) images to the correct location and to boot these images.
ZEPboot has no functionality to upload or download images.

ZEPboot provides support for two kind of applications (images):
a. An application (image) that provides upload functionality, this will be
   copied to slot 0 (see further) and will run from slot 0.
b. An application that provides upload functionality in slot 0, and a second
   application that will run from slot 1. In this case the application in slot 1
   is updated by running the image in slot 0. This setup allows execution of
   larger applications by keeping the image in slot 0 (upload functionality)
   smaller.

In the above two options images can be encrypted or not, but all images need to
be signed.

ZEPboot is a zephyr application with sources under bootloader/src.

## Limitations

The boot loader currently supports images with the following
characteristics:
* Built to run from flash/ram.
* Built to run from a fixed location (i.e., not position-independent).

## Images, slots and slotareas.

ZEPboot defines a number of slot areas where images can be stored. Each of the
slot areas is divided in three regions: a slot 0 region, a slot 1 region and a
swap status region (swpstat).

New images are placed in slot 1 and are then swapped with the image in slot 0 or
are left in slot 1 for in place execution. During the swap but also when leaving
the image in slot 1 encrypted images are decrypted.

In a classical swap setup images are compiled for execution from slot 0, in the
case of in place execution images are compiled for execution from slot 1. There
is also a third case where images are first copied to RAM and then executed from
RAM, in this case the image should be compiled for execution from RAM. In this
third case the bootloader uses a classical swap approach: images for RAM
execution are placed in slot 1, swapped to slot 0 (without decryption) and then
copied to RAM from slot 0.

The classical swap setup (and RAM execution images) allows images to be tested
and in case of an error during execution to restore the previous image.

In a setup where in place execution is used the application cannot be restored,
but there also is no need as the previous application can be rewritten.

Also multiple slot areas are supported in case the mcu needs the ability to
separately upgrade multiple images. In that case it is the responsability of
the image in slot area 0 to correctly start the images in the other slot areas.

## Image Format

Images contain a header part and the binary image.

The image header is a fixed size area of TLV (type length value) records that
describe the image properties:
* IMAGE_TYPE: not used at the moment,
* IMAGE_INFO: start, size, load_address, version,
* IMAGE_HASH: hash calculated over the image,
* IMAGE_EPUBKEY: public key used to generate the encryption key.

Prepended to the TLV area there is a header that contains information about the
TLV area (type, size, ...). Over the TLV area a signature is generated that is
stored in the TLV area header.

For a image to be valid it has to satisfy:
* The signature over the tlv area must be valid
* The hash calculated over the image must be equal to the hash stored in the TLV
  area.

## Image slot areas

The image slot areas are defined in [slotmap.c](../bootloader/src/slotmap.c)
In this file for each slot area that is needed the flash areas for slot0, slot1
and swapstat are defined.

# Bootloader security

ZEPboot uses Elliptical Curve Cryptography (ECC) to provide bootloader
security. The use of ECC allows to use the tinycrypt library for hashing,
encryption and signing.

## ZEPboot provides security on different levels:

a. Protection against image modifications by verifying a hash over the
(encrypted) image.

b. Protection against header modification (e.g. version number or image hash) by
signing the header with an ECC 256 signature.

c. Protection of IP during distribution of images by encrypting the
compiled binary using AES128-CTR.

To sign and encrypt images for use with ZEPboot, see: [imgtool](imgtool.md).

# Bootloader operation

The task of the bootloader is to copy new images in place, start the correct
image. The bootloader must do these tasks in a way that is protected against
power failures during the upgrade. To achieve this the bootloader executes the
process using a state machine where the advancement of the copy is stored in the
swap status area.

Each write to the swap status area is protected by a crc8 that guarantees a
correct value was written. When a wrong value is written it is ignored.

Each step of the swap process starts with a flash erase followed by a flash
copy. At the end of a step the next step is written to the swap status area.

This gurantees that:
* if the step being executed is interrupted, this step will be restarted at the
next reboot
* if the step being executed is interrupted during the writing of the next step
to the swap status area, the step will be restarted at the next reboot.

In each step a erase is performed, as a flash erase is done for a flash region
(a flash page or a multiple of a flash page) the steps need to process a flash
region at a time. The flash regions are called sectors and the sector size is
adjustable. In most cases it is safe to set the sector size equal to the flash
page size.

## Bootloader swap/decrypt process


The swap process that is carried out by the bootloader is done in several steps:

## move up

The move then starts by moving all of the sectors in slot 0 up one sector. It is
started from the end of the image: erase the sector after the last image sector
and copy the last image sector to the just erased sector. This is repeated
until the first image sector in slot 0 is moved up by one sector.

As a result of this operation we are left with the first sector of slot0 ready
to be erased.

## swap/decrypt

After the move up the swap is started. This swap consists of two steps for each
sector of slot0 or slot1 that needs to be swapped:

* Step 1: erase first sector of slot 0 and copy first sector of slot 1 to first
sector of slot 0.
* Step 2: erase first sector of slot 1 and copy second sector of slot 0 to the
first sector of slot 1. (The second sector of slot 0 contains the first sector
of the image in slot 0 after the move up).

These steps are then repeated until the end of the images is reached. During the
swap the data is also encrypted or decrypted.

## support for inplace execution of encrypted images

ZEPboot also provides support for encrypted images that are placed in the slot
it should be executed from. When such an image is detected the move up process
is executed followed by a swap/decrypt inside the slot using the same
methodology as the swap/decrypt above.

## bootloader limitation on image size

As the move command is first moving up a image by one sector the sector slot
cannot be fully used by the image. The largest image size is the slot size minus
the SECTORSIZE

## flash wear and SECTORSIZE

During an upgrade there are several erases of sectors:

a. the swap status area is deleted before every move.
b. each sector of slot1 is deleted twice: ones for placing the image in
the slot and ones during the swap.
c. each sector of slot0 is deleted twice: ones for performing the move up,
and one time during the swap.

So in a two slot solution each upgrade requires 2 erases for each sector. This
limits the amount of upgrades to half the number of times a flash sector can be
erased. This should be plenty even when using the bootloader during development.

remark: when using a encrypted inplace image each upgrade requires 3 erases for
each sector: one for placing the image in the slot, one for moving up and one
for swap/decrypt.

The number of sectors than can be processed by the bootloader is limited by the
SECTORSIZE, the size of the swap status area and the flash write block size.
Each write to the swap status area requires 4 bytes to be written, aligned to
the write block size. For each sector 3 writes are performed (one during move
up, and two during swap) + one write to start the move. So what does this mean:

```
example 1: nrf51822 with 256kB flash, 1kB page size, 4B write block size

* swap status area: 2kB (2 pages)
* maximum amount of sectors: 2048/(3*4) = 170
* SECTORSIZE = 1kB

---> MAX_IMAGE_SIZE = 170kB - 1kB = 169 kB

* Total space available for applications:

  256kB - 16kB (bootloader) - 2kB (swap status) - 2 kB (move up): 236 kB

```

```
example 2: nrf52832 with 512kB flash, 4kB page size, 8B write block size

* swap status area: 4kB (1 page)
* maximum amount of sectors: 4096/(3*8) = 170
* SECTORSIZE = 4kB

---> MAX_IMAGE_SIZE = 680kB (512kB)

* Total space available for applications:

  512kB - 16kB (bootloader) - 4kB (swap status) - 8kB (move up): 484 kB
```