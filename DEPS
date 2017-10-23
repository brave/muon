use_relative_paths = True

deps = {
  "vendor/node": "https://github.com/brave/node.git@7535fd34c36a40403743af5bf71c5b79b3f0bb6d",
  "vendor/ad-block": "https://github.com/brave/ad-block.git@0f51cf6c19146d8064c75bb63c7cdc52663f3d50",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@7b5fb79a32e8b3fa471d45a7d7be56bccc862de1",
  "vendor/hashset-cpp": "https://github.com/bbondy/hashset-cpp.git@3177fe41c7d73135a0703b13cf53346fa0cef2fd",
  "vendor/bloom-filter-cpp": "https://github.com/bbondy/bloom-filter-cpp.git@08e3d79206eee75e0e0932ca9c77fd1ee69f61a0",
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
