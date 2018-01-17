use_relative_paths = True

deps = {
  "vendor/node": "https://github.com/brave/node.git@488de2b7222f90667d373e40baf1761ccf27628e",
  "vendor/ad-block": "https://github.com/brave/ad-block.git@13c33eb0a9dd48c863c61c97ffe9e80414ae83d3",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@84695a8997c6e371f8dd2ddfaeb75e641e5e8ff2",
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
