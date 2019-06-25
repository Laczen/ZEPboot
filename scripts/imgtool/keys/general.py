"""General key class."""

import sys

class KeyClass(object):
    def emit_public(self, indent="\t"):
        encoded = self.get_public_key_bytearray()
        for count, b in enumerate(encoded):
            if count % 8 == 0:
                print("\n" + indent, end='')
            else:
                print(" ", end='')
            print("0x{:02x},".format(b), end='')

    def emit_private(self, indent="\t"):
        encoded = self.get_private_key_bytearray()
        for count, b in enumerate(encoded):
            if count % 8 == 0:
                print("\n" + indent, end='')
            else:
                print(" ", end='')
            print("0x{:02x},".format(b), end='')



