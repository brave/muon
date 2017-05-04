#!/usr/bin/env python

import errno
import os
import shutil
import subprocess
import sys
import tempfile

SOURCE_ROOT = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
VERBOSE_ENV_VAR='JS2ASAR'

def main():
  archive = sys.argv[1]
  folder_name = sys.argv[2]
  source_files = sys.argv[3:]

  output_dir = tempfile.mkdtemp()

  if os.getenv(VERBOSE_ENV_VAR, default=None) is not None:
    print "SRC:{}".format(source_files)
    print "FOLDER_NAME:{}".format(folder_name)
    print "OUTPUT:{}".format(output_dir)

  copy_files(folder_name, source_files, output_dir)
  call_asar(archive, os.path.join(output_dir, folder_name))
  shutil.rmtree(output_dir)


def copy_files(folder_name, source_files, output_dir):
  for source_file in source_files:
    sep = os.sep
    print "-----------------------"
    print source_file
    print "-----------------------"
    source_file = sep.join(source_file.split('/'))
    output_path = os.path.join(output_dir, source_file)

    if os.getenv(VERBOSE_ENV_VAR, default=None) is not None:
      print("SOURCE_FILE:{}".format(source_file))
      print("OUTPUT_PATH:{}".format(output_path))

    safe_mkdir(os.path.dirname(output_path))
    shutil.copy2(os.path.join(folder_name, source_file), output_path)


def call_asar(archive, output_dir):
  asar = os.path.join(SOURCE_ROOT, 'node_modules', '.bin', 'asar')
  if sys.platform in ['win32', 'cygwin']:
    asar += '.cmd'
  subprocess.check_call([asar, 'pack', output_dir, archive])


def safe_mkdir(path):
  try:
    os.makedirs(path)
  except OSError as e:
    if e.errno != errno.EEXIST:
      raise


if __name__ == '__main__':
  sys.exit(main())
