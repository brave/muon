use_relative_paths = True

deps = {
  "vendor/node": "https://github.com/brave/node.git@7316c73e7cebc2e3bc5e62ef696c1eb05a8d6d29",
  "vendor/ad-block": "https://github.com/brave/ad-block.git@d549e9afb5ec6ce01c6be3193b631db8bee53fe5",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@84695a8997c6e371f8dd2ddfaeb75e641e5e8ff2",
  "vendor/hashset-cpp": "https://github.com/bbondy/hashset-cpp.git@728fd67bc269765f5a566fb1d2fd9b04b632e68a",
  "vendor/bloom-filter-cpp": "https://github.com/bbondy/bloom-filter-cpp.git@b5509def04d1ecf60fdad62457a3bd09c457df90",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
}

hooks = [
  {
    'name': 'bootstrap',
    'pattern': '.',
    'action': ['python', 'src/electron/script/bootstrap.py'],
  },
  {
    'name': 'update_external_binaries',
    'pattern': '.',
    'action': ['python', 'src/electron/script/update-external-binaries.py']
  },
  {
    # Apply patches to chromium src
    'name': 'apply_patches',
    'pattern': '.',
    'action': ['python', 'src/electron/script/apply-patches.py'],
  }
]
