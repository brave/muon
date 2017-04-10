var ipc = require('ipc_utils')

var id = 1;
var focusId = 1;

var binding = {
  getCurrent: function () {
    var cb, getInfo;
    if (arguments.length == 1) {
      cb = arguments[0]
    } else {
      getInfo = arguments[0]
      cb = arguments[1]
    }

    var responseId = ++id
    ipc.once('chrome-windows-get-current-response-' + responseId, function(evt, win) {
      cb(win)
    })
    ipc.send('chrome-windows-get-current', responseId, getInfo)
  },

  getAll: function (getInfo, cb) {
    if (arguments.length == 1) {
      cb = arguments[0]
    } else {
      getInfo = arguments[0]
      cb = arguments[1]
    }

    var responseId = ++id
    ipc.once('chrome-windows-get-all-response-' + responseId, function(evt, win) {
      cb(win)
    })
    ipc.send('chrome-windows-get-all', responseId, getInfo)
  },

  create: function (createData, cb) {
    console.warn('chrome.windows.create is not supported yet')
  },

  update: function (windowId, updateInfo, cb) {
    var responseId = ++id
    cb && ipc.once('chrome-windows-update-response-' + responseId, function (evt, win) {
      cb(win)
    })
    ipc.send('chrome-windows-update', responseId, windowId, updateInfo)
  },

  get: function (windowId, getInfo, cb) {
    console.warn('chrome.windows.get is not supported yet')
  },

  getLastFocused: function (getInfo, cb) {
    console.warn('chrome.windows.getLastFocused is not supported yet')
  },

  remove: function (windowId, cb) {
    console.warn('chrome.windows.remove is not supported yet')
  },

  onCreated: {

    addListener: function (callback) {
      console.warn('chrome.windows.onCreated is not supported yet')
    },

    Filters: ['app', 'normal', 'panel', 'popup']
  },

  onRemoved: {

    addListener: function (callback) {
      console.warn('chrome.windows.onRemoved is not supported yet')
    },

    Filters: ['app', 'normal', 'panel', 'popup']
  },

  onFocusChanged: {

    addListener: function (cb) {

      if(cb.length != 1) {
        console.warn('Callback has wrong number of arguments.')
      }

      var responseId = ++focusId

      ipc.on('chrome-windows-focused-window-update-response-' + responseId, function (evt, responseId) {
        cb(responseId);
      })

      ipc.send('chrome-windows-focused-window-update', responseId)

    },

    Filters: ['app', 'normal', 'panel', 'popup']
  },

  WINDOW_ID_NONE: -1,
  WINDOW_ID_CURRENT: -2
};

exports.binding = binding;
