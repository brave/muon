var ipc = require('ipc_utils')

exports.didCreateDocumentElement = function() {
  // don't try to run if there is no window object
  if (!window)
    return

  var slice = [].slice;

  window.alert = function(message, title) {
    return ipc.send('window-alert', message, title);
  };

  window.confirm = function(message, title) {
    return ipc.sendSync('window-confirm', message, title);
  };
};

exports.binding = ipc;
