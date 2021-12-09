from __future__ import unicode_literals
from __future__ import print_function

import os
import unittest
import subprocess
import struct
import json
from base64 import b64encode, b64decode


BASE_DIR = os.path.dirname(os.path.dirname(
    os.path.dirname(os.path.abspath(__file__))))
WEBEID_APP = os.path.join(BASE_DIR, 'build', 'src', 'app', 'web-eid')


class Test(unittest.TestCase):

    def setUp(self):
        self.process = subprocess.Popen([WEBEID_APP, 'unused-argument'],
                                        stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        close_fds=True)

    def tearDown(self):
        self.process.wait()
        self.process.stdin.close()
        self.process.stdout.close()
        del self.process

    def test_0_quit(self):
        message = {
            'command': 'quit',
            'arguments': {}
        }
        response = self.exchange_message_with_app(message)
        self.assertFalse(response)

    def test_1_get_certificate(self):
        message = {
            'command': 'get-signing-certificate',
            'arguments': {
                'origin': 'https://ria.ee',
            }
        }
        response = self.exchange_message_with_app(message)
        self.assertIn('certificate', response)
        self.__class__.signingCertificate = response['certificate']

    def test_2_authenticate(self):
        message = {
            'command': 'authenticate',
            'arguments': {
                'challengeNonce': '12345678123456781234567812345678912356789123',
                'origin': 'https://ria.ee'
            }
        }
        response = self.exchange_message_with_app(message)
        self.assertIn('signature', response)
        # TODO: use a crypto library to validate the token signature.

    def test_3_sign(self):
        self.assertTrue(hasattr(self.__class__, 'signingCertificate'))
        message = {
            'command': 'sign',
            'arguments': {
                'hash': b64encode(b'x' * 48).decode('ascii'),
                'hashFunction': 'SHA-384',
                'origin': 'https://ria.ee',
                'certificate': self.__class__.signingCertificate,
            }
        }
        response = self.exchange_message_with_app(message)
        self.assertIn('signature', response)
        # TODO: use a crypto library to validate the signature.

    def exchange_message_with_app(self, message):
        print('Request:', json.dumps(message))
        version_message = self._read_response()
        self.assertIn('version', version_message)

        message = bytearray(json.dumps(message), 'utf-8')
        message_length = len(message)
        message_length = struct.pack('=I', message_length)
        self.process.stdin.write(message_length)
        self.process.stdin.write(message)
        self.process.stdin.flush()

        response = self._read_response()
        print('Response:', json.dumps(response))
        return response

    def _read_response(self):
        response_length = struct.unpack('=I', self.process.stdout.read(4))[0]
        response = self.process.stdout.read(response_length)
        return json.loads(response)


if __name__ == '__main__':
    unittest.main(failfast=True)
