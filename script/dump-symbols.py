#!/usr/bin/env python

import os
import sys

from lib.config import PLATFORM, SOURCE_ROOT, CHROMIUM_ROOT, OUT_DIR, product_name, project_name
from lib.util import execute, rm_rf


def main(destination):
  if PLATFORM == 'win32':
    register_required_dll()

  rm_rf(destination)
  (project_name, product_name) = get_names_from_gyp()

  if PLATFORM in ['darwin', 'linux']:
    generate_breakpad_symbols = os.path.join(SOURCE_ROOT, 'tools', 'posix',
                                             'generate_breakpad_symbols.py')
    if PLATFORM == 'darwin':
      start = os.path.join(OUT_DIR, '{0}.app'.format(product_name), 'Contents',
                           'MacOS', product_name)
    else:
      start = os.path.join(OUT_DIR, project_name)
    args = [
      '--build-dir={0}'.format(OUT_DIR),
      '--binary={0}'.format(start),
      '--symbols-dir={0}'.format(destination),
      '--libchromiumcontent-dir={0}'.format(CHROMIUM_ROOT),
      '--clear',
      '--jobs=16',
    ]
  else:
    generate_breakpad_symbols = os.path.join(SOURCE_ROOT, 'tools', 'win',
                                             'generate_breakpad_symbols.py')
    args = [
      '--symbols-dir={0}'.format(destination),
      '--jobs=16',
      os.path.relpath(OUT_DIR),
    ]

  execute([sys.executable, generate_breakpad_symbols] + args)


def register_required_dll():
  register = os.path.join(SOURCE_ROOT, 'tools', 'win',
                          'register_msdia80_dll.js')
  execute(['node.exe', os.path.relpath(register)]);


def get_names_from_gyp():
  return (project_name(), product_name())


if __name__ == '__main__':
  sys.exit(main(sys.argv[1]))
