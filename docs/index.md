<!--
  Copyright (c) 2018 Laczen

  SPDX-License-Identifier: Apache-2.0
-->
# ZEPboot

## Overview

ZEPboot is a secure bootloader for 32-bit MCUs.   ZEPboot defines a
common infrastructure for the bootloader, system flash layout on
microcontroller systems, and to provide a secure bootloader that enables
easy software upgrade.

Many idea's that are used in the implementation are coming from MCUboot,
a similar bootloader for 32-bit MCUs.

ZEPboot is developed on top of the zephyr RTOS.

## Contents

- General: this document
- [design](design.md): for the design
- [imgtool](imgtool.md): for creating images and key management
- [usage](usage.md): for bootloader usage, starting upgrade, testing images

