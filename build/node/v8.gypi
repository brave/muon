{
  'variables': {
    'icu_path': '../../../third_party/icu',
    # Using libc++ requires building for >= 10.7.
    'mac_deployment_target': '10.8',
    # Use the standard way of linking with msvc runtime.
    'win_use_allocator_shim': 0,
    # The V8 libraries.
    'v8_libraries': '["v8", "v8_snapshot", "v8_nosnapshot", "v8_external_snapshot", "v8_base", "v8_libbase", "v8_libplatform"]',
    # The icu libraries.
    'icu_libraries': '["icui18n", "icuuc"]',
    'v8_use_snapshot': 'true',
    'v8_use_external_startup_data': 1,
  },
  'target_defaults': {
    'msvs_disabled_warnings': [
        # class 'std::xx' needs to have dll-interface. Chrome turns this off
        # for component builds, and we need to too.
        4251,
        # The file contains a character that cannot be represented in these
        # current code page
        4819,
        # no matching operator delete found; memory will not be freed if
        # initialization throws an exception
        4291,
        # non dll-interface class used as base for dll-interface class
        4275,
        # 'GetVersionExW': was declared deprecated
        4996,
        # result of 32-bit shift implicitly converted to 64 bits
        4334,
    ],
    'xcode_settings': {
      'WARNING_CFLAGS': [
        '-Wno-deprecated-declarations',
      ],
      # Use C++11 library.
      'CLANG_CXX_LIBRARY': 'libc++',  # -stdlib=libc++
    },
    # Force exporting icu's symbols.
    'defines': [
      'U_COMBINED_IMPLEMENTATION',
      # Defining "U_COMBINED_IMPLEMENTATION" will add "explicit" for some
      # constructors, make sure it doesn' happen.
      'UNISTR_FROM_CHAR_EXPLICIT=',
      'UNISTR_FROM_STRING_EXPLICIT=',
      'U_NO_DEFAULT_INCLUDE_UTF_HEADERS=0',
    ],
    'defines!': [
      'U_STATIC_IMPLEMENTATION',
    ],
    'conditions': [
      ['OS=="linux" and target_arch=="arm"', {
        # Work around ODR violations.
        'ldflags!': [
          '-Wl,--detect-odr-violations',
        ],
      }],
    ],
    'target_conditions': [
      ['_type=="static_library" and _toolset=="target" and OS=="linux"', {
        'standalone_static_library': 1,
      }],
      ['_target_name in <(v8_libraries) + <(icu_libraries)', {
        'xcode_settings': {
          'DEAD_CODE_STRIPPING': 'NO',  # -Wl,-dead_strip
          'GCC_INLINES_ARE_PRIVATE_EXTERN': 'NO',
          'GCC_SYMBOLS_PRIVATE_EXTERN': 'NO',
        },
        'cflags!': [
          '-fvisibility=hidden',
          '-fdata-sections',
          '-ffunction-sections',
        ],
        'cflags_cc!': ['-fvisibility-inlines-hidden'],
      }],
      ['_target_name in <(v8_libraries) + ["mksnapshot"]', {
        'defines': [
          'V8_SHARED',
          'BUILDING_V8_SHARED',
        ],
      }],
    ],
  },
}
