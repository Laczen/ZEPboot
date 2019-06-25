# Copyright 2019 LaczenJMS
# Copyright 2018 Nordic Semiconductor ASA
# Copyright 2017 Linaro Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Image signing and management.
"""

from . import version as versmod
from intelhex import IntelHex
import hashlib
import struct
import os.path
import imgtool.keys as keys
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes

MIN_IMAGE_OFFSET = 512
TLV_AREA_MAGIC = 0x544c5641
TLVE_IMAGE_TYPE = 0x10
TLVE_IMAGE_INFO = 0x20
TLVE_IMAGE_HASH = 0x30
TLVE_IMAGE_EPUBKEY = 0x40

BIN_EXT = "bin"
INTEL_HEX_EXT = "hex"

STRUCT_ENDIAN_DICT = {
        'little': '<',
        'big':    '>'
}

class Image():
    def __init__(self, image_offset = MIN_IMAGE_OFFSET, slot_size = 0,
                 align = 1, slot_address = None, version = 0, endian="little"):
        self.image_offset = image_offset
        self.slot_size = slot_size
        self.align = align
        self.slot_address = slot_address
        self.image_address = None
        self.version = version or versmod.decode_version("0")
        self.endian = endian
        self.payload = []
        self.size = 0

    def __repr__(self):
        return "<image_offset={}, slot_size={}, align={}, slot_address={}, \
                Image version={}, endian={} format={}, \
                payloadlen=0x{:x}>".format(
                    self.image_offset,
                    self.slot_size,
                    self.align,
                    self.slot_address,
                    self.version,
                    self.endian,
                    self.__class__.__name__,
                    len(self.payload))

    def load(self, path):
        """Load an image from a given file"""
        ext = os.path.splitext(path)[1][1:].lower()
        if ext == INTEL_HEX_EXT:
            ih = IntelHex(path)
            self.payload = ih.tobinarray()
            self.image_address = ih.minaddr()
            self.slot_address = self.image_address - self.image_offset
            self.size = len(self.payload)
            self.payload = (b'\xff' * self.image_offset) + self.payload
        else:
            if self.slot_address is None:
                raise Exception("Input type bin requires a slot address")
            with open(path, 'rb') as f:
                self.payload = f.read()
            self.size = len(self.payload)
            # Add empty image header.
            self.payload = (b'\xff' * self.image_offset) + self.payload
            self.image_address = self.slot_address + self.image_offset

        self.check()

    def save(self, path):
        """Save an image from a given file"""
        ext = os.path.splitext(path)[1][1:].lower()
        if ext == INTEL_HEX_EXT:
            # input was in binary format, but HEX needs to know the base addr
            if self.slot_address is None:
                raise Exception("Input file does not provide a slot address")
            h = IntelHex()
            h.frombytes(bytes=self.payload, offset=self.slot_address)
            h.tofile(path, 'hex')
        else:
            with open(path, 'wb') as f:
                f.write(self.payload)

    def check(self):
        """Perform some sanity checking of the image."""
        # If there is a header requested, make sure that the image
        # starts with all 0xff.
        if self.image_offset > 0:
            if any(v != 0xff for v in self.payload[0:self.image_offset]):
                raise Exception("Padding requested, but image does not start \
                with 0xff")
        if self.slot_size > 0:
            padding = self.slot_size - len(self.payload)
            if padding < 0:
                msg = "Image size 0x{:x} exceeds requested size 0x{:x}".format(
                        len(self.payload), self.slot_size)
                raise Exception(msg)

    def create(self, signkey, encrkey):

        epubk = None
        if encrkey is not None:

            # Generate new encryption key
            tempkey = keys.EC256P1.generate()
            epubk = tempkey.get_public_key_bytearray()

            # Generate shared secret
            shared_secret = tempkey.gen_shared_secret(encrkey._get_public())

            # Key Derivation function: KDF1
            sha = hashlib.sha256()
            sha.update(shared_secret)
            sha.update(b'\x00\x00\x00\x00')
            plainkey = sha.digest()[:16]

            # Encrypt
            nonce = bytes([0] * 16)
            cipher = Cipher(algorithms.AES(plainkey), modes.CTR(nonce),
                            backend=default_backend())
            encryptor = cipher.encryptor()
            msg = bytes(self.payload[self.image_offset:])

            enc = encryptor.update(msg) + encryptor.finalize()
            self.payload = bytearray(self.payload)
            self.payload[self.image_offset:] = enc

        # Calculate the image hash.
        sha = hashlib.sha256()
        sha.update(self.payload[self.image_offset:])
        hash = sha.digest()

        self.add_header(hash, epubk, signkey)

    def add_header(self, hash, epubk, signkey):
        """Install the image header."""

        # Image info TLV
        e = STRUCT_ENDIAN_DICT[self.endian]
        fmt = (e +
            # type zb_tlv_img_info struct {
            'I' +   # Start uint32
            'I' +   # Size uint32
            'I' +   # Load Address uint32
            'BBHI'  # Vers ImageVersion
            ) # }
        img_info = struct.pack(fmt,
                self.image_offset,
                self.size,
                self.image_address,
                self.version.major,
                self.version.minor or 0,
                self.version.revision or 0,
                self.version.build or 0
                )

        tlv_area = struct.pack('B', TLVE_IMAGE_TYPE)
        tlv_area += struct.pack('B', 1)
        tlv_area += struct.pack('B', 0)

        tlv_area += struct.pack('B', TLVE_IMAGE_INFO)
        tlv_area += struct.pack('B', len(img_info))
        tlv_area += img_info

        tlv_area += struct.pack('B', TLVE_IMAGE_HASH)
        tlv_area += struct.pack('B', len(hash))
        tlv_area += hash

        if epubk is not None:
            tlv_area += struct.pack('B', TLVE_IMAGE_EPUBKEY)
            tlv_area += struct.pack('B', len(epubk))
            tlv_area += epubk

        sha = hashlib.sha256()
        sha.update(tlv_area)
        tlv_hash = sha.digest()
        tlv_signature = signkey.sign_prehashed(tlv_hash)

        # TLV area header
        e = STRUCT_ENDIAN_DICT[self.endian]
        fmt = (e +
            'I' +   # TLV AREA MAGIC
            'H' +   # Size uint16
            'B' +   # TLVA type
            'B'     # Signature type
            )

        # Pre calculation to allow padding
        hdr = struct.pack(fmt,
                TLV_AREA_MAGIC,
                0,
                0,
                0
                )

        hdr += tlv_signature

        # TLV header padding
        while ((len(hdr) % self.align) != 0) :
            hdr += struct.pack('B', 0xff)

        hdr_len = len(hdr) + len(tlv_area)

        # Final calculation
        hdr = struct.pack(fmt,
                TLV_AREA_MAGIC,
                hdr_len,
                0,
                0
                )

        hdr += tlv_signature

        # TLV header padding
        while ((len(hdr) % self.align) != 0) :
            hdr += struct.pack('B', 0xff)

        hdr += tlv_area

        self.payload = bytearray(self.payload)
        self.payload[0:len(hdr)] = hdr