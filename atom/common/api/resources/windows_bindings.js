var binding = require('binding').Binding.create('windows')

var ipc = require('ipc_utils')

var id = 1;

binding.registerCustomHook(function (bindingsAPI, extensionId) {
  var apiFunctions = bindingsAPI.apiFunctions
  var windows = bindingsAPI.compiledApi

  apiFunctions.setHandleRequest('getLastFocused', function (getInfo, cb) {
    console.warn('chrome.windows.getLastFocused is not supported yet')
  })

  apiFunctions.setHandleRequest('remove', function (windowId, cb) {
    console.warn('chrome.windows.remove is not supported yet')
  })

  apiFunctions.setHandleRequest('get', function (windowId, getInfo, cb) {
    console.warn('chrome.windows.get is not supported yet')
  })

  apiFunctions.setHandleRequest('create', function (createData, cb) {
    console.warn('chrome.windows.create is not supported yet')
  })

  apiFunctions.setHandleRequest('getCurrent', function () {
    var cb, getInfo
    if (arguments.length === 1) {
      cb = arguments[0]
    } else {
      getInfo = arguments[0]
      cb = arguments[1]
    }

    var responseId = ++id
    ipc.once('chrome-windows-get-current-response-' + responseId, function (evt, win) {
      cb(win)
    })
    ipc.send('chrome-windows-get-current', responseId, getInfo)
  })

  apiFunctions.setHandleRequest('getAll', function (getInfo, cb) {
    if (arguments.length === 1) {
      cb = arguments[0]
    } else {
      getInfo = arguments[0]
      cb = arguments[1]
    }

    var responseId = ++id
    ipc.once('chrome-windows-get-all-response-' + responseId, function (evt, win) {
      cb(win)
    })
    ipc.send('chrome-windows-get-all', responseId, getInfo)
  })

  apiFunctions.setHandleRequest('update', function (windowId, updateInfo, cb) {
    var responseId = ++id
    cb && ipc.once('chrome-windows-update-response-' + responseId, function (evt, win) {
      cb(win)
    })
    ipc.send('chrome-windows-update', responseId, windowId, updateInfo)
  })

  windows.onCreated = {
    addListener: function (callback) {
      console.warn('chrome.windows.onCreated is not supported yet')
    },

    Filters: function () {
      console.warn('chrome.windows.onCreated filters are not supported yet')
    }
  }

  windows.onRemoved = {
    addListener: function (callback) {
      console.warn('chrome.windows.onRemoved is not supported yet')
    },

    Filters: function () {
      console.warn('chrome.windows.onRemoved filters are not supported yet')
    }
  }

  windows.onFocusChanged = {
    addListener: function (cb) {
      ipc.send('register-chrome-window-focus', extensionId)
      ipc.on('chrome-window-focus', function (evt, windowId) {
        cb(windowId)
      })
    },

    Filters: function () {
      console.warn('chrome.windows.onFocusChanged filters are not supported yet')
    }
  }

  windows.WINDOW_ID_NONE = -1
  windows.WINDOW_ID_CURRENT = -2
})

exports.$set('binding', binding.generate())

