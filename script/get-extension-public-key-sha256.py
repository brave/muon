#!/usr/bin/python

# Extracts SubjectPublicKeyInfo and calculates the sha256 from it
# CRX file format: https://developer.chrome.com/extensions/crx
import struct
from hashlib import sha256
import base64
import sys
import os

assert len(sys.argv) == 2, 'Usage: ./script/get-extension-public-key-sha256.py <crx-path>'
extension_path = sys.argv[1]

file_size = os.path.getsize(extension_path)
f = open(extension_path, 'rb')

magic_number = f.read(4)
assert (magic_number == 'Cr24'), 'Not a valid crx file'

version_buffer = f.read(4)
version = struct.unpack_from('i', version_buffer)[0]
assert version == 2, 'Only version 2 CRX is currently supported'

public_key_len_buffer = f.read(4)
public_key_len = struct.unpack_from('i', public_key_len_buffer)[0]

# skip past the sig length
f.seek(4, 1)

public_key_info = f.read(public_key_len)
pk_hash = sha256(public_key_info).hexdigest()

f.seek(0)
all_crx_data = f.read(file_size)
all_crx_data_sha256 = sha256(all_crx_data).hexdigest()

def format_hex_data(pk_hash):
  return ' '.join('0x' + pk_hash[i:i+2] + ',' for i in xrange(0,len(pk_hash), 2))[:-1]

def format_hex_data(pk_hash):
  return ' '.join('0x' + pk_hash[i:i+2] + ',' for i in xrange(0,len(pk_hash), 2))[:-1]

def get_extension_id(pk_hash):
  return ''.join([chr(ord('a') + int(a, 16)) for a in pk_hash])[:32]

# In groups of 8 of '0x0e, '
def group_hex(hex):
  return '\n'.join(hex[i:i+48] for i in xrange(0,len(hex), 48))

# Used as the manifest key in the browser
print 'Manifest key: ', base64.b64encode(public_key_info)
print 'PK len: ', public_key_len
# Used by brave/electron to get the PK SHA256 and extension ID
print 'Public key: ', group_hex(format_hex_data(public_key_info.encode('hex')))
# Sent to the update server
print 'PK SHA256: ', group_hex(format_hex_data(pk_hash))
print 'extension ID:', get_extension_id(pk_hash)
print 'crx file data sha256 (for update server endpoint)', all_crx_data_sha256
