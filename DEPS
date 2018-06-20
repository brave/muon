use_relative_paths = True

deps = {
  "vendor/node": "https://github.com/brave/node.git@18f874a7222698fee07c977b84dc6082e114e185",
  "vendor/ad-block": "https://github.com/brave/ad-block.git@8d7c0bdbaa1d2c0a390859e58575a9e53cfafa94",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@77dab19f002d968a2ef841a1d317c57c695f7734",
  "vendor/hashset-cpp": "https://github.com/brave/hashset-cpp.git@edd90e8215ea34582811446a143a7c8063f535f0",
  "vendor/bloom-filter-cpp": "https://github.com/brave/bloom-filter-cpp.git@d511cf872ea1d650ab8dc4662f6036dac012d197",
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
