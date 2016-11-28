#!/usr/bin/env python

import os
import sys

from lib.config import OUT_DIR, get_electron_version


def main():
  create_version()


def create_version():
  version_path = os.path.join(OUT_DIR, 'version')
  with open(version_path, 'w') as version_file:
    version_file.write(get_electron_version())


if __name__ == '__main__':
  sys.exit(main())
