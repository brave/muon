#!/usr/bin/env python

import optparse
import sys
import glob
import os
import shutil


sys.path.append(os.path.join(os.path.dirname(__file__),
                             os.pardir, os.pardir,
                             "build"))
import gn_helpers

from lib.config import PLATFORM, SOURCE_ROOT, DIST_DIR, OUT_DIR, \
                       get_target_arch, get_zip_name, \
                       project_name, get_electron_version
from lib.util import scoped_cwd, make_zip, \
                     execute


def main():
  parser = optparse.OptionParser()

  parser.add_option('--inputs',
      help='GN format list of files to archive.')
  parser.add_option('--dir-inputs',
      help='GN format list of files to archive.')
  parser.add_option('--output', help='Path to output archive.')
  parser.add_option('--base-dir',
                    help='If provided, the paths in the archive will be '
                    'relative to this directory', default='.')

  options, _ = parser.parse_args()

  inputs = []
  if (options.inputs):
    parser = gn_helpers.GNValueParser(options.inputs)
    inputs = parser.ParseList()

  dir_inputs = []
  if options.dir_inputs:
    parser = gn_helpers.GNValueParser(options.dir_inputs)
    dir_inputs = parser.ParseList()

  output = options.output
  base_dir = options.base_dir

  # create_symbols()

  if PLATFORM == 'linux':
    strip_binaries()

  create_dist_zip(base_dir, output, inputs, dir_inputs)
  # create_symbols_zip()


def strip_binaries():
  for binary in TARGET_BINARIES[PLATFORM]:
    if binary.endswith('.so') or '.' not in binary:
      strip_binary(os.path.join(DIST_DIR, binary))


def strip_binary(binary_path):
    if get_target_arch() == 'arm':
      strip = 'arm-linux-gnueabihf-strip'
    else:
      strip = 'strip'
    execute([strip, binary_path])


def create_symbols():
  destination = os.path.join(DIST_DIR, '{0}.breakpad.syms'.format(project_name()))
  dump_symbols = os.path.join(SOURCE_ROOT, 'script', 'dump-symbols.py')
  execute([sys.executable, dump_symbols, destination])

  if PLATFORM == 'darwin':
    dsyms = glob.glob(os.path.join(OUT_DIR, '*.dSYM'))
    for dsym in dsyms:
      shutil.copytree(dsym, os.path.join(DIST_DIR, os.path.basename(dsym)))
  elif PLATFORM == 'win32':
    pdbs = glob.glob(os.path.join(OUT_DIR, '*.pdb'))
    for pdb in pdbs:
      shutil.copy2(pdb, DIST_DIR)


def create_dist_zip(base_dir, zip_file, files, dirs):
  with scoped_cwd(base_dir):
    make_zip(zip_file, files, dirs)


def create_symbols_zip():
  dist_name = get_zip_name(project_name(), get_electron_version(), 'symbols')
  zip_file = os.path.join(DIST_DIR, dist_name)
  licenses = ['LICENSE', 'LICENSES.chromium.html', 'version']

  with scoped_cwd(DIST_DIR):
    dirs = ['{0}.breakpad.syms'.format(project_name())]
    make_zip(zip_file, licenses, dirs)

  if PLATFORM == 'darwin':
    dsym_name = get_zip_name(project_name(), get_electron_version(), 'dsym')
    with scoped_cwd(DIST_DIR):
      dsyms = glob.glob('*.dSYM')
      make_zip(os.path.join(DIST_DIR, dsym_name), licenses, dsyms)
  elif PLATFORM == 'win32':
    pdb_name = get_zip_name(project_name(), get_electron_version(), 'pdb')
    with scoped_cwd(DIST_DIR):
      pdbs = glob.glob('*.pdb')
      make_zip(os.path.join(DIST_DIR, pdb_name), pdbs + licenses, [])


if __name__ == '__main__':
  sys.exit(main())
