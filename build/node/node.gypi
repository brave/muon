{
  'includes': [
    'v8.gypi',
  ],
  'variables': {
    'clang_use_chrome_plugins': 0,
    'openssl_fips': '',
    'openssl_no_asm': 1,
    'use_openssl_def': 0,
    'OPENSSL_PRODUCT': 'libopenssl.a',
    'node_use_boringssl': 'true',
    'node_release_urlbase': '',
    'node_byteorder': '<!(node <(DEPTH)/electron/tools/get-endianness.js)',
    'node_target_type': 'shared_library',
    'node_install_npm': 'false',
    'node_prefix': '',
    'node_shared': 'true',
    'node_shared_cares': 'false',
    'node_shared_http_parser': 'false',
    'node_shared_libuv': 'false',
    'node_shared_openssl': 'true',
    'node_shared_v8': 'true',
    'node_shared_zlib': 'false',
    'node_tag': '',
    'node_use_dtrace': 'false',
    'node_use_etw': 'false',
    'node_use_mdb': 'false',
    'node_use_openssl': 'true',
    'node_use_perfctr': 'false',
    'node_use_v8_platform': 'false',
    'node_use_bundled_v8': 'false',
    'node_enable_d8': 'false',
    'uv_library': 'static_library',
    'uv_parent_path': 'vendor/node/deps/uv',
    'uv_use_dtrace': 'false',
    'V8_BASE': '',
    'v8_postmortem_support': 'false',
    'v8_enable_i18n_support': 0,
    'v8_inspector': 'false',
    'library': 'static_library',
  },
  'target_defaults': {
    'default_configuration': 'Release',
    'configurations': {

      #
      # Concrete configurations
      #
      # 'Node_Debug': {
      #   'inherit_from': ['Common_Base', 'x86_Base', 'Debug_Base'],
      # },
      # 'Node_Release': {
      #   'inherit_from': ['Common_Base', 'x86_Base', 'Release_Base'],
      # },
    },
    'target_conditions': [
      ['_target_name in ["libuv", "http_parser", "boringssl", "openssl-cli", "cares", "node", "zlib"]', {
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
        'defines': [
          'NODE_WANT_INTERNALS=1',
          'NODE_SHARED_MODE',
        ],
        'cflags': [
          '-fvisibility=default',
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
            'variables': {
              'conditions': [
                ['target_arch=="ia32"', {
                  'reference_symbols': [
                    '_u_errorName_56',
                    '_ubidi_setPara_56',
                    '_ucsdet_getName_56',
                    '_uidna_openUTS46_56',
                    '_ulocdata_close_56',
                    '_unorm_normalize_56',
                    '_uregex_matches_56',
                    '_uscript_getCode_56',
                    '_uspoof_open_56',
                    '_usearch_setPattern_56',
                    '?createInstance@Transliterator@icu_56@@SAPAV12@ABVUnicodeString@2@W4UTransDirection@@AAW4UErrorCode@@@Z',
                    '??0MeasureFormat@icu_56@@QAE@ABVLocale@1@W4UMeasureFormatWidth@@AAW4UErrorCode@@@Z',
                  ],
                }, {
                  'reference_symbols': [
                    'u_errorName_56',
                    'ubidi_setPara_56',
                    'ucsdet_getName_56',
                    'uidna_openUTS46_56',
                    'ulocdata_close_56',
                    'unorm_normalize_56',
                    'uregex_matches_56',
                    'uspoof_open_56',
                    'usearch_setPattern_56',
                    '?createInstance@Transliterator@icu_56@@SAPEAV12@AEBVUnicodeString@2@W4UTransDirection@@AEAW4UErrorCode@@@Z',
                    '??0MeasureFormat@icu_56@@QEAA@AEBVLocale@1@W4UMeasureFormatWidth@@AEAW4UErrorCode@@@Z',
                  ],
                }],
              ],
            },

            # Fix the linking error with icu.
             # 'conditions': [
              # ['libchromiumcontent_component==0', {
              #
              #   'msvs_settings': {
              #     'VCLinkerTool': {
              #       # There is nothing like "whole-archive" on Windows, so we
              #       # have to manually force some objets files to be included
              #       # by referencing them.
              #       'ForceSymbolReferences': [ '<@(reference_symbols)' ],  # '/INCLUDE'
              #     },
              #   },
              # }],
            # ],
          }],
          # ['OS=="linux" and libchromiumcontent_component==0', {
          #   # Prevent the linker from stripping symbols.
          #   'ldflags': [
          #     '-Wl,--whole-archive',
          #     '<@(libchromiumcontent_v8_libraries)',
          #     '-Wl,--no-whole-archive',
          #   ],
          # }, {
          #   'libraries': [ '<@(libchromiumcontent_v8_libraries)' ],
          # }],
        ],
      }],
      ['_target_name=="boringssl"', {
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
      4716,  # 'function' must return a value
    ],
  },

  'conditions': [
    ['OS=="win"', {
      'target_defaults': {
        'configurations': {
          'Debug': {
           'inherit_from': ['Common_Base', 'x64_Base', 'Debug_Base'],
          },
          'Release': {
            'inherit_from': ['Common_Base', 'x64_Base', 'Release_Base'],
          },
        },
      },
    }],  # OS=="win"
  ]
}


