{
  'includes': [ 'v8.gypi' ],
  'variables': {
    'clang_use_chrome_plugins': 0,
    'openssl_fips': '',
    'openssl_no_asm': 1,
    'use_openssl_def': 0,
    'OPENSSL_PRODUCT': 'libopenssl.a',
    'node_release_urlbase': '',
    'node_byteorder': '<!(node <(DEPTH)/electron/tools/get-endianness.js)',
    'node_target_type': 'shared_library',
    'node_install_npm': 'false',
    'node_prefix': '',
    'node_shared': 'true',
    'node_shared_cares': 'false',
    'node_shared_http_parser': 'false',
    'node_shared_libuv': 'false',
    'node_shared_openssl': 'false',
    'node_shared_zlib': 'false',
    'node_tag': '',
    'node_use_dtrace': 'false',
    'node_use_etw': 'false',
    'node_use_mdb': 'false',
    'node_use_openssl': 'true',
    'node_use_perfctr': 'false',
    'node_use_v8_platform': 'false',
    'node_use_bundled_v8': 'false',
    'node_use_chromium_v8': 'true',
    'node_enable_d8': 'false',
    'uv_library': 'static_library',
    'uv_parent_path': 'vendor/node/deps/uv',
    'uv_use_dtrace': 'false',
    'V8_BASE': '',
    'v8_postmortem_support': 'false',
    'v8_inspector': 'false',
    'library': 'static_library',
    'msvs_use_common_release': 0,
    'icu_small': 'false',
  },
  'target_defaults': {
    'default_configuration': 'Release',
    'configurations': {
      'Release': {
        'conditions': [
          ['target_arch == "x64"', {
            'inherit_from': ['x64_Base'],
          }]
        ]
      },
      'Debug': {
        'conditions': [
          ['target_arch == "x64"', {
            'inherit_from': ['x64_Base'],
          }]
        ]
      }
    },
    'target_conditions': [
      ['_target_name in ["libuv", "http_parser", "openssl", "openssl-cli", "cares", "node", "zlib"]', {
        'msvs_disabled_warnings': [
          4003,  # not enough actual parameters for macro 'V'
          4013,  # 'free' undefined; assuming extern returning int
          4018,  # signed/unsigned mismatch
          4054,  #
          4055,  # 'type cast' : from data pointer 'void *' to function pointer
          4057,  # 'function' : 'volatile LONG *' differs in indirection to slightly different base types from 'unsigned long *'
          4065,  # switch statement contains 'default' but no 'case' labels
          4189,  #
          4131,  # uses old-style declarator
          4133,  # incompatible types
          4146,  # unary minus operator applied to unsigned type, result still unsigned
          4164,  # intrinsic function not declared
          4152,  # function/data pointer conversion in expression
          4206,  # translation unit is empty
          4204,  # non-constant aggregate initializer
          4210,  # nonstandard extension used : function given file scope
          4214,  # bit field types other than int
          4232,  # address of dllimport 'free' is not static, identity not guaranteed
          4291,  # no matching operator delete found
          4295,  # array is too small to include a terminating null character
          4311,  # 'type cast': pointer truncation from 'void *const ' to 'unsigned long'
          4389,  # '==' : signed/unsigned mismatch
          4456,  # declaration of 'm' hides previous local declaration
          4457,  # declaration of 'message' hides function parameter
          4459,  # declaration of 'wq' hides global declaration
          4477,  # format string '%.*s' requires an argument of type 'int'
          4505,  # unreferenced local function has been removed
          4701,  # potentially uninitialized local variable 'sizew' used
          4703,  # potentially uninitialized local pointer variable 'req' used
          4706,  # assignment within conditional expression
          4804,  # unsafe use of type 'bool' in operation
          4996,  # this function or variable may be unsafe.
          4716,  # not returning a value
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'WarnAsError': 'false',
          },
        },
        'xcode_settings': {
          'GCC_TREAT_WARNINGS_AS_ERRORS': 'NO',
          'WARNING_CFLAGS': [
            '-Wno-unknown-warning-option',
            '-Wno-parentheses-equality',
            '-Wno-unused-function',
            '-Wno-sometimes-uninitialized',
            '-Wno-pointer-sign',
            '-Wno-sign-compare',
            '-Wno-string-plus-int',
            '-Wno-unused-variable',
            '-Wno-deprecated-declarations',
            '-Wno-return-type',
            '-Wno-gnu-folding-constant',
            '-Wno-shift-negative-value',
            '-Wno-varargs', # https://git.io/v6Olj
          ],
        },
        'conditions': [
          ['OS=="linux"', {
            'cflags': [
              '-Wno-parentheses-equality',
              '-Wno-unused-function',
              '-Wno-sometimes-uninitialized',
              '-Wno-pointer-sign',
              '-Wno-string-plus-int',
              '-Wno-unused-variable',
              '-Wno-unused-value',
              '-Wno-deprecated-declarations',
              '-Wno-return-type',
              '-Wno-shift-negative-value',
              '-Wno-varargs', # https://git.io/v6Olj
              '-Wno-string-conversion',
              # Required when building as shared library.
              '-fPIC',
            ],
          }],
        ],
      }],
      ['_target_name=="node"', {
        'include_dirs': [
          '../v8',
          '../v8/include',
        ],
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
        'conditions': [
          ['OS=="mac"', {
            'xcode_settings': {
              'GCC_INLINES_ARE_PRIVATE_EXTERN': 'NO',
              'GCC_SYMBOLS_PRIVATE_EXTERN': 'NO',
            },
          }],
          ['OS=="mac" and node_target_type=="shared_library"', {
            # -all_load is the "whole-archive" on macOS.
            'xcode_settings': {
              'LD_DYLIB_INSTALL_NAME': '@rpath/libnode.dylib',
              # 'DYLIB_INSTALL_NAME_BASE': '@loader_path',
              'OTHER_LDFLAGS': [ '-Wl,-all_load' ],
            },
          }],
          ['OS=="win"', {
            'defines': [
              'WIN32',
              # we don't really want VC++ warning us about
              # how dangerous C functions are...
              '_CRT_SECURE_NO_DEPRECATE',
              # ... or that C implementations shouldn't use
              # POSIX names
              '_CRT_NONSTDC_NO_DEPRECATE',
              # Make sure the STL doesn't try to use exceptions
              '_HAS_EXCEPTIONS=0',
              'BUILDING_V8_SHARED=1',
              'BUILDING_UV_SHARED=1',
            ],
            # Fix passing fd across modules, see |osfhandle.h| for more.
            'sources': [
              '../../atom/node/osfhandle.cc',
              '../../atom/node/osfhandle.h',
            ],
            'include_dirs': [
              '../../atom/node',
            ],
            # Node is using networking API but linking with this itself.
            'libraries': [ '-lwinmm.lib' ],
            'msvs_settings': {
              'VCLinkerTool': {
                'EnableCOMDATFolding': '1', # disable
                'OptimizeReferences': '1', # disable
              },
            },
          }],
        ],
      }],
      ['_target_name=="openssl"', {
        'xcode_settings': {
          'DEAD_CODE_STRIPPING': 'YES',  # -Wl,-dead_strip
          'GCC_INLINES_ARE_PRIVATE_EXTERN': 'YES',
          'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES',
        },
        'cflags': [
          '-fvisibility=hidden',
        ],
      }],
      ['_target_name=="libuv"', {
        'conditions': [
          ['OS=="win"', {
            # Expose libuv's symbols.
            'defines': [
              'BUILDING_UV_SHARED=1',
            ],
          }],  # OS=="win"
        ],
      }],
    ],
    'msvs_cygwin_shell': 0, # Strangely setting it to 1 would make building under cygwin fail.
    'msvs_disabled_warnings': [
      4005,  # (node.h) macro redefinition
      4091,  # (node_extern.h) '__declspec(dllimport)' : ignored on left of 'node::Environment' when no variable is declared
      4189,  # local variable is initialized but not referenced
      4201,  # (uv.h) nameless struct/union
      4267,  # conversion from 'size_t' to 'int', possible loss of data
      4273,  # http://crbug.com/154421
      4302,  # (atldlgs.h) 'type cast': truncation from 'LPCTSTR' to 'WORD'
      4456,
      4457,  # http://crbug.com/154421
      4458,  # (atldlgs.h) declaration of 'dwCommonButtons' hides class member
      4503,  # decorated name length exceeded, name was truncated
      4800,  # (v8.h) forcing value to bool 'true' or 'false'
      4819,  # The file contains a character that cannot be represented in the current code page
      4838,  # (atlgdi.h) conversion from 'int' to 'UINT' requires a narrowing conversion
      4996,  # (atlapp.h) 'GetVersionExW': was declared deprecated
      4716,  # 'function' must return a value,
      4251, # class 'std::xx' needs to have dll-interface.
    ],
  },
}


