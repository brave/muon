use_relative_paths = True

deps = {
  "vendor/node": "https://github.com/brave/node.git@542fab459cc8361405f7c78c965b27e0ee0f0092",
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
