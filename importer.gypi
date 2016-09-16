{
  'variables': {
    'importer_sources': [
      'atom/browser/api/atom_api_importer.cc',
      'atom/browser/api/atom_api_importer.h',
      'atom/browser/importer/profile_writer.cc',
    ],
  },
  'conditions': [
    ['OS=="mac"', {
      'variables': {
        'importer_libraries': [
          '<(libchromiumcontent_dir)/libbookmarks_browser.a',
          '<(libchromiumcontent_dir)/libcomponent_metrics_proto.a',
          '<(libchromiumcontent_dir)/libfavicon_base.a',
          '<(libchromiumcontent_dir)/libgoogle_apis.a',
          '<(libchromiumcontent_dir)/libgoogle_core_browser.a',
          '<(libchromiumcontent_dir)/libhistory_core_browser.a',
          '<(libchromiumcontent_dir)/libquery_parser.a',
          '<(libchromiumcontent_dir)/liburl_formatter.a',
          '<(libchromiumcontent_dir)/libsearch_engines.a',
        ],
        'importer_include_dir': [
          '<(libchromiumcontent_dir)/gen/components/strings',
        ],
      }
    }],
    ['OS=="linux"', {
      'variables': {
        'importer_libraries': [
          '<(libchromiumcontent_dir)/libbookmarks_browser.a',
          '<(libchromiumcontent_dir)/libcomponent_metrics_proto.a',
          '<(libchromiumcontent_dir)/libfavicon_base.a',
          '<(libchromiumcontent_dir)/libgoogle_apis.a',
          '<(libchromiumcontent_dir)/libgoogle_core_browser.a',
          '<(libchromiumcontent_dir)/libhistory_core_browser.a',
          '<(libchromiumcontent_dir)/libquery_parser.a',
          '<(libchromiumcontent_dir)/liburl_formatter.a',
          '<(libchromiumcontent_dir)/libsearch_engines.a',
        ],
        'importer_include_dir': [
          '<(libchromiumcontent_dir)/gen/components/strings',
          '/usr/local/include/nss',
          '/usr/local/include/nspr',
        ],
      }
    }],
    ['OS=="win"', {
      'variables': {
        'importer_libraries': [
          '<(libchromiumcontent_dir)/bookmarks_browser.lib',
          '<(libchromiumcontent_dir)/component_metrics_proto.lib',
          '<(libchromiumcontent_dir)/favicon_base.lib',
          '<(libchromiumcontent_dir)/google_apis.lib',
          '<(libchromiumcontent_dir)/google_core_browser.lib',
          '<(libchromiumcontent_dir)/history_core_browser.lib',
          '<(libchromiumcontent_dir)/query_parser.lib',
          '<(libchromiumcontent_dir)/url_formatter.lib',
          '<(libchromiumcontent_dir)/search_engines.lib',
          '-lesent.lib',
        ],
        'importer_include_dir': [
          '<(libchromiumcontent_dir)/gen/components/strings',
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
