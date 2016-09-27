var services = {
  passwordSavingEnabled: {
    get: function(details, cb) {
      // TODO(bridiver) - should be pref enabled
      cb({level_of_control: "not_controllable", value: true});
    },
    set: function() {}
  },
  autofillEnabled: {
    get: function(details, cb) {
      // TODO(bridiver) - should be pref enabled
      cb({level_of_control: "not_controllable", value: true});
    },
    set: function() {}
  }
}

var binding = {
  services
}

exports.binding = binding;
