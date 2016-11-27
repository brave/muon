#!/usr/bin/env python

import glob
import os
import re
import shutil
import subprocess
import sys
import stat

from lib.config import PLATFORM, SOURCE_ROOT, CHROMIUM_ROOT, DIST_DIR, OUT_DIR, \
                       get_target_arch, get_chromedriver_version, get_zip_name, \
                       project_name, product_name, get_electron_version
from lib.util import scoped_cwd, rm_rf, make_zip, \
                     execute


TARGET_BINARIES = {
  'darwin': [
  ],
  'win32': [
    '{0}.exe'.format(project_name()),  # 'electron.exe'
    'electron_resources.pak',
    'd3dcompiler_47.dll',
    'libEGL.dll',
    'libGLESv2.dll',
    'ffmpeg.dll',
    'node.dll',
    'xinput1_3.dll',
    'natives_blob.bin',
    'snapshot_blob.bin',
    'widevinecdmadapter.dll',
  ],
  'linux': [
    project_name(),  # 'electron'
    'electron_resources.pak',
    'icudtl.dat',
    'libnode.so',
    'natives_blob.bin',
    'snapshot_blob.bin',
    'libwidevinecdmadapter.so',
  ],
}

TARGET_DIRECTORIES = {
  'darwin': [
    '{0}.app'.format(product_name()),
  ],
  'win32': [
    'resources',
    'locales',
  ],
  'linux': [
    'resources',
    'locales',
  ],
}


def main():
  rm_rf(DIST_DIR)
  os.makedirs(DIST_DIR)

  # create_symbols()
  copy_binaries()
  copy_chrome_binary('chromedriver')
  copy_chrome_binary('mksnapshot')
  generate_licenses()
  copy_license()

  if PLATFORM == 'linux':
    strip_binaries()

  create_version()
  create_dist_zip()
  create_chrome_binary_zip('chromedriver', get_chromedriver_version())
  create_chrome_binary_zip('mksnapshot', get_electron_version())
  # create_symbols_zip()


def copy_binaries():
  for binary in TARGET_BINARIES[PLATFORM]:
    shutil.copy2(os.path.join(OUT_DIR, binary), DIST_DIR)

  for directory in TARGET_DIRECTORIES[PLATFORM]:
    shutil.copytree(os.path.join(OUT_DIR, directory),
                    os.path.join(DIST_DIR, directory),
                    symlinks=True)


def copy_chrome_binary(binary):
  if PLATFORM == 'win32':
    binary += '.exe'
  src = os.path.join(OUT_DIR, binary)
  dest = os.path.join(DIST_DIR, binary)

  # Copy file and keep the executable bit.
  shutil.copyfile(src, dest)
  os.chmod(dest, os.stat(dest).st_mode | stat.S_IEXEC)


def generate_licenses():
  file_template = os.path.join(SOURCE_ROOT, 'resources', 'about_credits.tmpl')
  entry_template = os.path.join(SOURCE_ROOT, 'resources',
                                'about_credits_entry.tmpl')
  licenses_py = os.path.join(CHROMIUM_ROOT, 'tools', 'licenses.py')
  target = os.path.join(DIST_DIR, 'LICENSES.chromium.html')
  subprocess.check_call([sys.executable, licenses_py, 'credits', target,
                         '--file-template', file_template,
                         '--entry-template', entry_template])

def copy_license():
  shutil.copy2(os.path.join(SOURCE_ROOT, 'LICENSE'), DIST_DIR)


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


def create_version():
  version_path = os.path.join(DIST_DIR, 'version')
  with open(version_path, 'w') as version_file:
    version_file.write(get_electron_version())


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


def create_dist_zip():
  dist_name = get_zip_name(project_name(), get_electron_version())
  zip_file = os.path.join(DIST_DIR, dist_name)

  with scoped_cwd(DIST_DIR):
    files = TARGET_BINARIES[PLATFORM] +  ['LICENSE', 'LICENSES.chromium.html',
                                          'version']
    dirs = TARGET_DIRECTORIES[PLATFORM]
    make_zip(zip_file, files, dirs)


def create_chrome_binary_zip(binary, version):
  dist_name = get_zip_name(binary, version)
  zip_file = os.path.join(DIST_DIR, dist_name)

  with scoped_cwd(DIST_DIR):
    files = ['LICENSE', 'LICENSES.chromium.html']
    if PLATFORM == 'win32':
      files += [binary + '.exe']
    else:
      files += [binary]
    make_zip(zip_file, files, [])


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
