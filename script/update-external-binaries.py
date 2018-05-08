#!/usr/bin/env python

import errno
import sys
import os
import urllib2

from lib.config import get_target_arch
from lib.util import safe_mkdir, rm_rf, extract_zip, tempdir, download


VERSION = 'v1.1.0'
SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
FRAMEWORKS_URL = 'https://github.com/electron/electron-frameworks/releases' \
                 '/download/' + VERSION


def main():
  os.chdir(SOURCE_ROOT)
  version_file = os.path.join(SOURCE_ROOT, 'external_binaries', '.version')

  if (is_updated(version_file, VERSION)):
    return

  rm_rf('external_binaries')
  safe_mkdir('external_binaries')

  if sys.platform == 'darwin':
    download_and_unzip('Mantle')
    download_and_unzip('ReactiveCocoa')
    download_and_unzip('Squirrel')
  elif sys.platform in ['cygwin', 'win32']:
    download_and_unzip('directxsdk-' + get_target_arch())

  with open(version_file, 'w') as f:
    f.write(VERSION)


def is_updated(version_file, version):
  existing_version = ''
  try:
    with open(version_file, 'r') as f:
      existing_version = f.readline().strip()
  except IOError as e:
    if e.errno != errno.ENOENT:
      raise
  return existing_version == version


def download_and_unzip(framework):
  zip_path = download_framework(framework)
  if zip_path:
    extract_zip(zip_path, 'external_binaries')


def download_framework(framework):
  filename = framework + '.zip'
  url = FRAMEWORKS_URL + '/' + filename
  download_dir = tempdir(prefix='electron-')
  path = os.path.join(download_dir, filename)
  filedata = urllib2.urlopen(url)
  datatowrite = filedata.read()
  with open(path, 'wb') as f:
    f.write(datatowrite)

  return path


if __name__ == '__main__':
  sys.exit(main())
