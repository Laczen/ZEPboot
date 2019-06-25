<!--
  Copyright (c) 2018 Laczen

  SPDX-License-Identifier: Apache-2.0
-->
# ZEPboot - A work in progress - expect rough edges

## Overview

ZEPboot is a secure bootloader for 32-bit MCUs. ZEPboot defines a
common infrastructure for the bootloader, system flash layout on
microcontroller systems, and provides a secure bootloader that enables
easy software upgrade.

Many idea's that are used in the implementation are coming from MCUboot,
a similar bootloader for 32-bit MCUs.



ZEPboot is developed on top of the zephyr RTOS.

## Documentation

Project documentation is located in the [docs](./docs/index.md) folder.

## Testing the bootloader with nrfjprog

Testing the bootloader with nrfjprog on nrf51/nrf52 devices requires 6 steps:

### Step 1: Compile and program the bootloader

```
west build -d build_zepboot -s zepboot/bootloader/ -b nrf52_pca10040 -t flash
```

REMARK: The configuration file creates a bootloader with logging enabled, this creates a bootloader of about 22Kb. With logging disabled this size reduces to about 14.5Kb.

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