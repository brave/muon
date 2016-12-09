const binding = require('binding').Binding.create('tabViewInternal').generate()

exports.$set(
    'TabViewInternal',
    binding);
exports.$set('binding', binding);
