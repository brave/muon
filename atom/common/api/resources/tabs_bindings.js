var binding = require('binding').Binding.create('tabs');

var atom = requireNative('atom').GetBinding();
var messaging = require('messaging');
var tabsNatives = requireNative('tabs');
var OpenChannelToTab = tabsNatives.OpenChannelToTab;
var process = requireNative('process');
var sendRequestIsDisabled = process.IsSendRequestDisabled();
var forEach = require('utils').forEach;
var extensionId = process.GetExtensionId();
var ipc = require('ipc_utils')
var lastError = require('lastError');
var chrome = requireNative('chrome').GetChrome();

let id = 0

const query = function (queryInfo, cb) {
  var responseId = ++id
  ipc.once('chrome-tabs-query-response-' + responseId, cb)
  ipc.send('chrome-tabs-query', responseId, queryInfo)
}

binding.registerCustomHook(function(bindingsAPI, extensionId) {
  var apiFunctions = bindingsAPI.apiFunctions
  var tabs = bindingsAPI.compiledApi;

  tabs.onUpdated = {
    addListener: function (cb) {
      ipc.send('register-chrome-tabs-updated', extensionId);
      ipc.on('chrome-tabs-updated', function(evt, tabId, changeInfo, tab) {
        cb(tabId, changeInfo, tab);
      })
    }
  },

  tabs.onCreated = {
    addListener: function (cb) {
      ipc.send('register-chrome-tabs-created', extensionId);
      ipc.on('chrome-tabs-created', function(evt, tab) {
        cb(tab)
      })
    }
  },

  tabs.onRemoved = {
    addListener: function (cb) {
      ipc.send('register-chrome-tabs-removed', extensionId);
      ipc.on('chrome-tabs-removed', function (evt, tabId, removeInfo) {
        cb(tabId, removeInfo)
      })
    }
  },

  tabs.onActivated = {
    addListener: function (cb) {
      ipc.send('register-chrome-tabs-activated', extensionId)
      ipc.on('chrome-tabs-activated', function (evt, tabId, activeInfo) {
        cb(activeInfo)
      })
    }
  },

  tabs.onSelectionChanged = {
    addListener: function (cb) {
      ipc.send('register-chrome-tabs-activated', extensionId)
      ipc.on('chrome-tabs-activated', function (evt, tabId, selectInfo) {
        cb(tabId, selectInfo)
      })
    }
  },

  tabs.onActiveChanged = {
    addListener: function (cb) {
      ipc.send('register-chrome-tabs-activated', extensionId)
      ipc.on('chrome-tabs-activated', function (evt, tabId, selectInfo) {
        cb(tabId, selectInfo)
      })
    }
  },

  apiFunctions.setHandleRequest('connect', function (tabId, connectInfo) {
    var name = ''
    var frameId = -1;
    if (connectInfo) {
      name = connectInfo.name || name
      frameId = connectInfo.frameId
      if (typeof frameId == 'undefined' || frameId < 0)
        frameId = -1
    }
    var portId = OpenChannelToTab(tabId, frameId, extensionId, name)
    return messaging.createPort(portId, name)
  })

  apiFunctions.setHandleRequest('sendRequest',
                                function(tabId, request, responseCallback) {
    if (sendRequestIsDisabled)
      throw new Error(sendRequestIsDisabled);
    var port = tabs.connect(tabId, {name: messaging.kRequestChannel})
    messaging.sendMessageImpl(port, request, responseCallback)
  })

  apiFunctions.setHandleRequest('sendMessage',
      function(tabId, message, options, responseCallback) {
    var connectInfo = {
      name: messaging.kMessageChannel
    }
    if (options) {
      forEach(options, function(k, v) {
        connectInfo[k] = v
      })
    }

    var port = tabs.connect(tabId, connectInfo)
    messaging.sendMessageImpl(port, message, responseCallback)
  })

  apiFunctions.setHandleRequest('query', function (queryInfo, cb) {
    var responseId = ++id
    query(queryInfo, function (evt, tab, error) {
      if (error) {
        lastError.run('tabs.query', error, '', () => { cb(null) })
      } else {
        cb(tab)
      }
    })
  })

  apiFunctions.setHandleRequest('captureVisibleTab', function (windowId, options, cb) {
    // return an empty image url
    cb('data:image/gif;base64,R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==')
  })

  apiFunctions.setHandleRequest('getSelected', function (windowId, cb) {
    windowId = windowId || -2
    query({windowId: windowId, active: true}, function (evt, tabs, error) {
      if (error) {
        lastError.run('tabs.getSelected', error, '', () => { cb(null) })
      } else {
        cb(tabs[0])
      }
    })
  })

  apiFunctions.setHandleRequest('get', function (tabId, cb) {
    var responseId = ++id
    ipc.once('chrome-tabs-get-response-' + responseId, function (evt, tab, error) {
      if (error) {
        lastError.run('tabs.get', error, '', () => { cb(null) })
      } else {
        cb(tab)
      }
    })
    ipc.send('chrome-tabs-get', responseId, tabId)
  })

  apiFunctions.setHandleRequest('getCurrent', function (cb) {
    var responseId = ++id
    ipc.once('chrome-tabs-get-current-response-' + responseId, function (evt, tab, error) {
      if (error) {
        lastError.run('tabs.getCurrent', error, '', () => { cb(null) })
      } else {
        cb(tab)
      }
    })
    ipc.send('chrome-tabs-get-current', responseId)
  })

  apiFunctions.setHandleRequest('update', function (tabId, updateProperties, cb) {
    var responseId = ++id
    cb && ipc.once('chrome-tabs-update-response-' + responseId, function (evt, tab, error) {
      if (error) {
        lastError.run('tabs.update', error, '', () => { cb(null) })
      } else {
        cb(tab)
      }
    })
    ipc.send('chrome-tabs-update', responseId, tabId, updateProperties)
  })

  apiFunctions.setHandleRequest('remove', function (tabIds, cb) {
    var responseId = ++id
    cb && ipc.once('chrome-tabs-remove-response-' + responseId, function (evt, error) {
      if (error) {
        lastError.run('tabs.remove', error, '', cb)
      } else {
        cb()
      }
    })
    ipc.send('chrome-tabs-remove', responseId, tabIds)
  })

  apiFunctions.setHandleRequest('create', function (createProperties, cb) {
    var responseId = ++id
    cb && ipc.once('chrome-tabs-create-response-' + responseId, function (evt, tab, error) {
      if (error) {
        lastError.run('tabs.remove', error, '', () => { cb(null) })
      } else {
        cb(tab)
      }
    })
    ipc.send('chrome-tabs-create', responseId, createProperties)
  })

  apiFunctions.setHandleRequest('executeScript', function (tabId, details, cb) {
    var responseId = ++id
    tabId = tabId || -2
    cb && ipc.once('chrome-tabs-execute-script-response-' + responseId, function (evt, error, on_url, results) {
      if (error) {
        lastError.run('tabs.executeScript', error, '', cb, results)
      } else {
        cb(results)
      }
    })
    ipc.send('chrome-tabs-execute-script', responseId, extensionId, tabId, details)
  })
})

exports.$set('binding', binding.generate())
