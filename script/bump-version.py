#!/usr/bin/env python

import os
import re
import sys

from lib.config import SOURCE_ROOT, get_electron_version
from lib.util import execute, parse_version, scoped_cwd


def main():
  if len(sys.argv) != 2 or sys.argv[1] == '-h':
    print 'Usage: bump-version.py [<version> | major | minor | patch]'
    return 1

  option = sys.argv[1]
  increments = ['major', 'minor', 'patch', 'build']
  if option in increments:
    version = get_electron_version()
    versions = parse_version(version.split('-')[0])
    versions = increase_version(versions, increments.index(option))
  else:
    versions = parse_version(option)

  version = '.'.join(versions[:3])

  with scoped_cwd(SOURCE_ROOT):
    update_package_json(version)
    tag_version(version)


def increase_version(versions, index):
  for i in range(index + 1, 4):
    versions[i] = '0'
  versions[index] = str(int(versions[index]) + 1)
  return versions


def update_package_json(version):
  package_json = 'package.json'
  with open(package_json, 'r') as f:
    lines = f.readlines()

  for i in range(0, len(lines)):
    line = lines[i];
    if 'version' in line:
      lines[i] = '  "version": "{0}",\n'.format(version)
      break

  with open(package_json, 'w') as f:
    f.write(''.join(lines))


def tag_version(version):
  execute(['git', 'commit', '-a', '-m', 'Bump v{0}'.format(version)])


if __name__ == '__main__':
  sys.exit(main())
