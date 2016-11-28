#!/usr/bin/env python

import os
import re
import sys

from lib.config import SOURCE_ROOT
from lib.util import execute, get_electron_version, parse_version, scoped_cwd


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
    tag_version(version)


def increase_version(versions, index):
  for i in range(index + 1, 4):
    versions[i] = '0'
  versions[index] = str(int(versions[index]) + 1)
  return versions


def tag_version(version):
  execute(['git', 'commit', '-a', '-m', 'Bump v{0}'.format(version)])


if __name__ == '__main__':
  sys.exit(main())
