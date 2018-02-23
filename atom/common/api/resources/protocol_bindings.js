var ipc = require('ipc_utils')
var IpcStream = require('electron-ipc-stream')

var protocol = {
  registerStringProtocol: function (scheme, handler) {
    ipc.on('chrome-protocol-handler-' + scheme, function(evt, request, requestId) {
      const cb = (data) => {
        ipc.send('chrome-protocol-handled-' + requestId, data)
      }
      handler(request, cb)
    })
    ipc.send('register-protocol-string-handler', scheme)
  },

  registerStreamProtocol: function (scheme, handler) {
    ipc.on('chrome-protocol-stream-handler-' + scheme, function (evt, request, requestId) {
      const cb = (data) => {
        const headers = data.headers
        const statusCode = data.statusCode
        const ipcStream = new IpcStream(`chrome-protocol-stream-handled-${scheme}-${requestId}-stream`)
        data.data.pipe(ipcStream)
        ipc.send(`chrome-protocol-stream-handled-${scheme}-${requestId}`, {headers, statusCode})
      }
      handler(request, cb)
    })
    ipc.send('register-protocol-stream-handler', scheme)
  }
}

exports.binding = protocol;
