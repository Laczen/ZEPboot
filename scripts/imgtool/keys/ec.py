"""
EC key management
"""

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric import utils
from cryptography.hazmat.primitives.hashes import SHA256

from .general import KeyClass

class ECUsageError(Exception):
    pass

class EC256P1Public(KeyClass):
    def __init__(self, key, endian):
        self.key = key

    def shortname(self):
        return "ec256"

    def get_public_key_size(self):
        return 64

    def _unsupported(self, name):
        raise ECUsageError("Operation {} requires private key".format(name))

    def _get_public(self):
        return self.key

    def get_public_key_bytearray(self):
        pubkey = self.key.public_key().public_numbers().x.to_bytes(32,'big')
        pubkey += self.key.public_key().public_numbers().y.to_bytes(32,'big')
        return pubkey

    def get_public_bytes(self):

        return self._get_public().public_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PublicFormat.SubjectPublicKeyInfo)

    def export_private(self, path, passwd=None):
        self._unsupported('export_private')

    def export_public(self, path):
        """Write the public key to the given file."""
        pem = self._get_public().public_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PublicFormat.SubjectPublicKeyInfo)
        with open(path, 'wb') as f:
            f.write(pem)

class EC256P1(EC256P1Public):
    """
    Wrapper around an EC private key.
    """

    def __init__(self, key):
        """key should be an instance of EllipticCurvePrivateKey"""
        self.key = key

    @staticmethod
    def generate():
        pk = ec.generate_private_key(
                ec.SECP256R1(),
                backend=default_backend())

        return EC256P1(pk)

    def _get_public(self):
        return self.key.public_key()

    def get_private_key_size(self):
        return 32

    def get_private_key_bytearray(self):
        return self.key.private_numbers().private_value.to_bytes(32, 'big')

    def export_private(self, path, passwd=None):
        """Write the private key to the given file, protecting it with the optional password."""
        if passwd is None:
            enc = serialization.NoEncryption()
        else:
            enc = serialization.BestAvailableEncryption(passwd)
        pem = self.key.private_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PrivateFormat.PKCS8,
                encryption_algorithm=enc)
        with open(path, 'wb') as f:
            f.write(pem)
        print("Done export")

    def sign_prehashed(self, hash):

        sig_der = self.key.sign(hash,ec.ECDSA(utils.Prehashed(SHA256())))
        signature = utils.decode_dss_signature(sig_der)
        signature_bin = signature[0].to_bytes(32,'big')
        signature_bin += signature[1].to_bytes(32,'big')
        return signature_bin

    def gen_shared_secret(self, publickey):
        return self.key.exchange(ec.ECDH(), publickey)