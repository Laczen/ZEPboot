<!--
  Copyright (c) 2018 Laczen

  SPDX-License-Identifier: Apache-2.0
-->

# Using the bootloader

Communication with ZEPboot is achieved by writing commands to the start of the
last sector of slot1 (for multi-images there are as many slot1 area's as there
are slotareas). The commands are a 4 byte sequence of which the last byte is a
one byte crc calculated over the 3 previous bytes:

	CMD = CMD1 | CMD2 | CMD3 | CRC8

Only CMD1 is used for communication with ZEPboot.

## Requesting a temporary swap

When a image needs to be tested a temporary swap is created. In this case if the
image fails a reboot will restore the previous image.

Do a temporary swap: `CMD1 = 0x10, CMD = 0x10000089`

## Making a temporary swap permanent

When during the testing of an image the image is considered ok, it can be made
permanent.

Make a temporary swap permanent: `CMD1 = 0x01, CMD = 0x01000040`

## Requesting a permanent swap

When an upgrade image has already been tested and is considered ok, a permanent
swap can be requested.

Do a permanent swap: `CMD1 = 0x11, CMD = 0x110000E2`

## Requesting ZEPboot to boot slot 0

In the case were ZEPboot is used to run images from slot 1 it is needed to boot
the image in slot 0 to allow images to be loaded to slot 1 using the image in
slot 0. In this case a request can be made to boot the image in slot 0.

Do a boot of image in slot 0: `CMD1 = 0x20, CMD = 0x20000068`

## Testing the bootloader with nrfjprog

Testing the bootloader with nrfjprog on nrf51/nrf52 devices requires 6 steps:

### Step 1: Compile and program the bootloader

```
west build -d build_zepboot -s zepboot/bootloader/ -b nrf52_pca10040 -t flash
```

### Step 2: Compile a test program (e.g. hello_world)

Creating a program for ZEPboot requires the following additions to prf.conf
(nrf52_pca10040, image running from slot 0):

```
CONFIG_FLASH_LOAD_OFFSET=0xc200
CONFIG_FLASH_LOAD_SIZE=0x32000
```

or (nrf52_pca10040, image running from slot 1):

```
CONFIG_FLASH_LOAD_OFFSET=0x3e200
CONFIG_FLASH_LOAD_SIZE=0x32000
```

```
west build -s zephyr/samples/hello_world/ -b nrf52_pca10040
```

### Step 3: Sign and encrypt the test program:

```
./scripts/imgtool.py sign -io 0x200 -ss 0x32000 -a 8 -v 0.0.1 -sk root-ec256.pem -ek boot-ec256.pem build/zephyr/zephyr.hex build/signed.hex
```

### Step 4: Adapt the signed.hex file and load it

In case a image for execution from slot 1 has been generated, this image can be loaded using nrfjprog:

```
nrfjprog --program build/signed.hex --sectorerase
```

In case the created hex file was generated for slot 0 it is required to adapt the starting address to upload it to slot 1 using nrfjprog (for nrf52_pca10040):

```
objcopy --change-addresses 0x32000 build/signed.hex build/signedoff.hex
```

Upload the new signedoff.hex using nrfjprog:

```
nrfjprog --program build/signedoff.hex --sectorerase
```

### Step 5: Write the swap command

Remark: the command has to be written in reverse order

```
nrfjprog --memwr 0x6f000 --val 0xE2000011 --verify
```

### Step 6: Hit reset


