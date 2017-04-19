{
  'includes': [ 'node.gypi' ],
  'target_defaults': {
    'target_conditions': [
      ['_target_name=="v8_libbase" or _target_name=="v8_base"', {
        'defines': [
          'BUILDING_V8_SHARED',
          'BUILDING_V8_BASE_SHARED',
        ],
      }],
      ['_target_name=="v8_external_snapshot"', {
        'defines': [
          'BUILDING_V8_SHARED',
        ],
      }],
    ],
  },
}
