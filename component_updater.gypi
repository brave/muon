{
  'variables': {
    'component_updater_sources': [
      'brave/browser/api/brave_api_component_updater.cc',
      'brave/browser/api/brave_api_component_updater.h',
      'brave/browser/component_updater/extension_installer_traits.cc',
      'brave/browser/component_updater/extension_installer_traits.h',
      'brave/browser/component_updater/brave_component_updater_configurator.cc',
      'brave/browser/component_updater/brave_component_updater_configurator.h',
      'brave/browser/component_updater/default_extensions.h',
    ],
    'component_updater_js_sources': [
      'lib/browser/api/component-updater.js',
    ],
  },
  'conditions': [
    ['OS=="mac" or OS=="linux"', {
      'variables': {
        'component_updater_libraries': [
         '<(libchromiumcontent_dir)/libupdate_client.a',
         '<(libchromiumcontent_dir)/libclient_update_protocol.a',
         '<(libchromiumcontent_dir)/libcomponent_updater.a',
         '<(libchromiumcontent_dir)/libcourgette_lib.a',
         '<(libchromiumcontent_dir)/libversion_info.a',
         '<(libchromiumcontent_dir)/liblzma_sdk.a',
         '<(libchromiumcontent_dir)/libminizip.a',
         '<(libchromiumcontent_dir)/libzip.a',
        ]
      }
    }],
    ['OS=="win"', {
      'variables': {
        'component_updater_libraries': [
          '<(libchromiumcontent_dir)/client_update_protocol.lib',
          '<(libchromiumcontent_dir)/component_updater.lib',
          '<(libchromiumcontent_dir)/courgette_lib.lib',
          '<(libchromiumcontent_dir)/lzma_sdk.lib',
          '<(libchromiumcontent_dir)/minizip.lib',
          '<(libchromiumcontent_dir)/update_client.lib',
          '<(libchromiumcontent_dir)/version_info.lib',
          '<(libchromiumcontent_dir)/zip.lib',
          '<(libchromiumcontent_dir)/zlib.lib',
        ],
      },
    }],
  ]
}
