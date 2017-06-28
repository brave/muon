var ipc = require('ipc_utils')

var protocol = {
  registerStringProtocol: function (scheme, handler) {
    ipc.on('chrome-protocol-handler-' + scheme, function(evt, request, requestId) {
      const cb = (data) => {
        ipc.send('chrome-protocol-handled-' + requestId, data)
      }
      handler(request, cb)
    })
    ipc.send('register-protocol-string-handler', scheme)
  }
}

exports.binding = protocol;
