{
  'variables': {
    'update_client_sources': [
      'atom/browser/api/atom_api_update_client.cc',
      'atom/browser/api/atom_api_update_client.h',
    ],
  },
  'conditions': [
    ['OS=="mac" or OS=="linux"', {
      'variables': {
        'update_client_libraries': [
         '<(libchromiumcontent_dir)/libupdate_client.a',
         '<(libchromiumcontent_dir)/libclient_update_protocol.a',
         '<(libchromiumcontent_dir)/libcomponent_updater.a',
         '<(libchromiumcontent_dir)/libcourgette_lib.a',
         '<(libchromiumcontent_dir)/libversion_info.a',
         '<(libchromiumcontent_dir)/liblzma_sdk.a',
        ]
      }
    }],
    ['OS=="win"', {
      'variables': {
        'update_client_libraries': [
          '<(libchromiumcontent_dir)/update_client.lib',
          '<(libchromiumcontent_dir)/client_update_protocol.lib',
          '<(libchromiumcontent_dir)/libcomponent_updater.lib',
          '<(libchromiumcontent_dir)/courgette_lib.lib',
          '<(libchromiumcontent_dir)/version_info.lib',
          '<(libchromiumcontent_dir)/lzma_sdk.lib',
        ],
      },
    }],
  ],
  'target_defaults': {
    'defines': [
      'GOOGLE_PROTOBUF_NO_RTTI'
    ],
  },
}
