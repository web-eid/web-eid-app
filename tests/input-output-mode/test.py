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
        self.process = subprocess.Popen(WEBEID_APP, stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE, close_fds=True)

    def tearDown(self):
        self.process.wait()
        self.process.stdin.close()
        self.process.stdout.close()
        del self.process

    def test_1_get_certificate(self):
        message = {
            'command': 'get-certificate',
            'arguments': {
                'type': 'sign',
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
                'nonce': '1' * 32,
                'origin': 'https://ria.ee',
                'origin-cert': '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'
            }
        }
        response = self.exchange_message_with_app(message)
        self.assertIn('auth-token', response)
        self.assert_aud_field_present(response)
        # TODO: use a JWT library to validate the authentication token.

    def test_2_authenticate_without_origin_cert(self):
        message = {
            'command': 'authenticate',
            'arguments': {
                'nonce': '1' * 32,
                'origin': 'https://ria.ee',
                'origin-cert': None
            }
        }
        response = self.exchange_message_with_app(message)
        self.assertIn('auth-token', response)
        self.assert_aud_field_present(response)

    def assert_aud_field_present(self, response):
        authtoken = response['auth-token']
        payload = b64decode(authtoken.split('.')[1] + '===')
        self.assertIn(b'aud', payload)

    def test_3_sign(self):
        self.assertTrue(hasattr(self.__class__, 'signingCertificate'))
        message = {
            'command': 'sign',
            'arguments': {
                'doc-hash': b64encode(b'x' * 48).decode('ascii'),
                'hash-algo': 'SHA-384',
                'origin': 'https://ria.ee',
                'user-eid-cert': self.__class__.signingCertificate,
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
