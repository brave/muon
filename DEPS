use_relative_paths = True

deps = {
  "vendor/node": "https://github.com/brave/node.git@86deba03b1b5a9c391dd8dfaec9f02cdf59b59f1",
  "vendor/ad-block": "https://github.com/brave/ad-block.git@5377ddea390a0641d99556e0cf9d9c15b8ef275c",
  "vendor/tracking-protection": "https://github.com/brave/tracking-protection.git@bad1cf834508eab25ba6051d04edfb4d68e45401",
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
