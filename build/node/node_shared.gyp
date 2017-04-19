{
  'targets': [{
    'target_name': 'node_shared',
    'type': 'shared_library',

    'include_dirs': [
      '<(DEPTH)/electron/vendor/node/src',
      '<(DEPTH)/electron/vendor/node/deps/cares/include',
      '<(DEPTH)/electron/vendor/node/deps/uv/include',
      '<(DEPTH)/v8/include',
      '<(DEPTH)/v8',
    ],

    'sources': [
      '<(DEPTH)/electron/build/node/nodedll-main.cc'
    ],

    'defines': [
      'BORINGSSL',
      'NODE_SHARED_MODE',
      'BUILDING_V8_SHARED',
      'BUILDING_V8_BASE_SHARED',
    ],

    'dependencies': [
      '<(DEPTH)/electron/vendor/node/node.gyp:node',
      '<(DEPTH)/v8/src/v8.gyp:v8_external_snapshot',
      '<(DEPTH)/v8/src/v8.gyp:v8_base',
    ],

    'msvs_settings': {
      'VCLinkerTool': {
        'ForceSymbolReferences': [
          'node_module_register',
        ],
      },
    },

    'link_settings': {
      'libraries': [
        '<(PRODUCT_DIR)/obj/third_party/boringssl/boringssl.lib',
        '<(PRODUCT_DIR)/obj/third_party/boringssl/boringssl_asm.lib',
      ],
    },
  }]
}
