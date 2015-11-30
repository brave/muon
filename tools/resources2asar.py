#!/usr/bin/env python

import os
import shutil
import subprocess
import sys
import tempfile


SOURCE_ROOT = os.path.dirname(os.path.dirname(__file__))


def main():
  archive = sys.argv[1]
  resource_files = sys.argv[2:]

  output_dir = tempfile.mkdtemp()
  for src in resource_files:
    shutil.copy(src, output_dir)
  call_asar(archive, output_dir)
  shutil.rmtree(output_dir)


def call_asar(archive, resource_source_dir):
  asar = os.path.join(SOURCE_ROOT, 'node_modules', 'asar', 'bin', 'asar')
  subprocess.check_call([find_node(), asar, 'pack', resource_source_dir, archive])


def find_node():
  WINDOWS_NODE_PATHs = [
    'C:/Program Files (x86)/nodejs',
    'C:/Program Files/nodejs',
  ] + os.environ['PATH'].split(os.pathsep)

  if sys.platform in ['win32', 'cygwin']:
    for path in WINDOWS_NODE_PATHs:
      full_path = os.path.join(path, 'node.exe')
      if os.path.exists(full_path):
        return full_path
  return 'node'

if __name__ == '__main__':
  sys.exit(main())
