var binding = require('binding').Binding.create('cookies')
var ipc = require('ipc_utils')
var lastError = require('lastError')

binding.registerCustomHook(function (bindingsAPI, extensionId) {
  var apiFunctions = bindingsAPI.apiFunctions
  // var cookies = bindingsAPI.compiledApi

  apiFunctions.setHandleRequest('getAll', function (details, cb) {
    var responseId = ipc.guid()
    cb && ipc.once('chrome-cookies-getall-response-' + responseId, function (evt, error, cookielist) {
      if (error) {
        lastError.run('cookies.getall', error, '', () => { cb(null) })
      } else {
        cb(cookielist)
      }
    })

    ipc.send('chrome-cookies-getall', responseId, details)
  })

  apiFunctions.setHandleRequest('get', function (details, cb) {
    var responseId = ipc.guid()
    cb && ipc.once('chrome-cookies-get-response-' + responseId, function (evt, error, cookie) {
      if (error) {
        lastError.run('cookies.get', error, '', () => { cb(null) })
      } else {
        cb(cookie)
      }
    })

    ipc.send('chrome-cookies-get', responseId, details)
  })

  apiFunctions.setHandleRequest('set', function (details, cb) {
    var responseId = ipc.guid()
    cb && ipc.once('chrome-cookies-set-response-' + responseId, function (evt, error, cookie) {
      if (error) {
        lastError.run('cookies.set', error, '', () => { cb(null) })
      } else {
        cb(cookie)
      }
    })

    ipc.send('chrome-cookies-set', responseId, details)
  })
})

exports.$set('binding', binding.generate())
