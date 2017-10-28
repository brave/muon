use_relative_paths = True

deps = {
  "vendor/node": "https://github.com/brave/node.git@7535fd34c36a40403743af5bf71c5b79b3f0bb6d",
  "vendor/ad-block": "https://github.com/brave/ad-block.git@8642be202f5bd00ee84af43a0ca29a3ddad0594c",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@eea148c33e5a465d2fda86e21742381c313947de",
  "vendor/hashset-cpp": "https://github.com/bbondy/hashset-cpp.git@728fd67bc269765f5a566fb1d2fd9b04b632e68a",
  "vendor/bloom-filter-cpp": "https://github.com/bbondy/bloom-filter-cpp.git@b5509def04d1ecf60fdad62457a3bd09c457df90",
  "vendor/requests": "https://github.com/kennethreitz/requests@e4d59bedfd3c7f4f254f4f5d036587bcd8152458",
  "vendor/boto": "https://github.com/boto/boto@f7574aa6cc2c819430c1f05e9a1a1a666ef8169b",
  "vendor/python-patch": "https://github.com/svn2github/python-patch@a336a458016ced89aba90dfc3f4c8222ae3b1403",
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
