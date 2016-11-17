include_rules = [
  "+crypto",
  "+gpu",
  "+net",
  "+pdf",
  "+printing",
  "+sql",
  # Browser, renderer, common and tests access V8 for various purposes.
  "-v8",
  "+v8/include",

  # Limit what we include from nacl.
  "-native_client",

  # The subdirectories in chrome/ will manually allow their own include
  # directories in chrome/ so we disallow all of them.
  "-chrome",
  "+chrome/common",
  # "+chrome/test",
  "+components/content_settings/core/common",
  # "+components/error_page/common",
  # "+components/omnibox/common",
  # "+components/url_formatter",
  # "+components/variations",
  "+content/public/common",
  # "+content/public/test",
  "+mojo/common",
  "+mojo/public",

  # Don't allow inclusion of these other libs we shouldn't be calling directly.
  "-webkit",
  "-tools",

  "-crypto/third_party",

  # Allow inclusion of WebKit API files.
  "+third_party/WebKit/public/platform",
  "+third_party/WebKit/public/web",

  # Allow inclusion of third-party code:
  "+third_party/hunspell",

  "+skia",
  "+third_party/skia",
  "+third_party/skia/include",
  "+third_party/skia/include/core",

  "+third_party/icu/source/common/unicode",
  "+third_party/icu/source/i18n/unicode",

  "+ui",

  "+electron/vendor/brightray",
]

use_relative_paths = True

deps = {
  "vendor/node": "https://github.com/brave/node.git@a57e6ac8d99e84bce640bf854c34d4ff0338f915",
  "vendor/native_mate": "https://github.com/zcbenz/native-mate.git@b5e5de626c6a57e44c7e6448d8bbaaac475d493c",
}

hooks = [
  {
    'name': 'bootstrap',
    'pattern': '.',
    'action': ['python', 'script/bootstrap.py'],
  },
  {
    'name': 'update_external_binaries',
    'pattern': '.',
    'action': ['python', 'script/update-external-binaries.py']
  }
]
