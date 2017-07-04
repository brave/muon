use_relative_paths = True

deps = {
  "vendor/ad-block": "https://github.com/brave/ad-block.git@eb1e8fc085df98de749cbfe27fa5ec607d180b75",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@ca040734abd9895b98660fdf41f0c8c95c7b1173",
  "vendor/hashset-cpp": "https://github.com/bbondy/hashset-cpp.git@6a6ca168ea5343b8090ca76fee9d76852fbb4ef5",
  "vendor/bloom-filter-cpp": "https://github.com/bbondy/bloom-filter-cpp.git@5e5a53e9cf9503fce7b8c2887c6a8655eb254d40",
  "vendor/node": "https://github.com/brave/node.git@f51b9ab8ff446ca7b13be0de1bc12b854b23938d",
  "vendor/native_mate": "https://github.com/zcbenz/native-mate.git@ad0fd825663932ee3fa29ff935dfec99933bdd8c",
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
