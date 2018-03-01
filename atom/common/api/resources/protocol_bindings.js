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
  },

  registerStreamProtocol: function (scheme, handler) {
    ipc.on('chrome-protocol-stream-handler-' + scheme, function (evt, request, requestId) {
      console.log('protocol', 'chrome-protocol-stream-handler-' + scheme, requestId)
      const cb = (res) => {
        if (typeof res.pipe === 'function') {
          res = { data: res }
        }

        res.headers = res.headers || {}
        res.statusCode = res.statusCode || 200

        ipc.send(`chrome-protocol-stream-handled-${requestId}-headers`, { headers: res.headers, statusCode: res.statusCode })

        res.data
          .on('data', (chunk) => {
            const data = JSON.stringify(chunk)
            ipc.send(`chrome-protocol-stream-handled-${requestId}-stream-data`, data)
          })
          .on('error', (err) => {
            const data = { message: err.message, stack: err.stack }
            ipc.send(`chrome-protocol-stream-handled-${requestId}-stream-error`, data)
          })
          .on('end', () => {
            ipc.send(`chrome-protocol-stream-handled-${requestId}-stream-finish`)
          })
      }
      handler(request, cb)
    })
    ipc.send('register-protocol-stream-handler', scheme)
  }
}

exports.binding = protocol;
