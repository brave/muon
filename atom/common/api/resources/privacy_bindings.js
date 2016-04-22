var services = {
  passwordSavingEnabled: {
    get: function(details, cb) {
      cb({level_of_control: "not_controllable"});
    }
  },
  autofillEnabled: {
    get: function(details, cb) {
      cb({level_of_control: "not_controllable"});
    }
  }
}

var binding = {
  services
}

exports.binding = binding;
