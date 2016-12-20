{
 'variables': {
   'icu_path': '../../../third_party/icu',
   'icu_use_data_file_flag': 1,
   # Using libc++ requires building for >= 10.7.
   'mac_deployment_target': '10.8',
   # Use the standard way of linking with msvc runtime.
   'win_use_allocator_shim': 0,
   'v8_enable_i18n_support': 1,
   # The V8 libraries.
   'v8_libraries': '["v8", "v8_snapshot", "v8_nosnapshot", "v8_external_snapshot", "v8_base", "v8_libbase", "v8_libplatform"]',
   'v8_use_snapshot': 'true',
   'v8_use_external_startup_data': 1,
   'icu_libraries': '["icui18n", "icuuc"]',
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
   'target_conditions': [
     ['_type=="static_library" and _toolset=="target" and OS=="linux"', {
       'standalone_static_library': 1,
     }],
     ['_target_name in <(icu_libraries)', {
      'type': 'static_library',
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
       'cflags': [
         '-fvisibility=default',
       ],
       'ldflags!': [
         '-Wl,--gc-sections',
       ],
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
