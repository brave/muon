'use strict'

const ipcRenderer = require('ipc_utils')

var WEB_VIEW_EVENTS = {
  'load-start': ['url', 'isMainFrame', 'isErrorPage'],
  'load-commit': ['url', 'isMainFrame'],
  'did-attach': ['tabId'],
  'did-detach': [],
  'did-finish-load': ['validatedURL'],
  'did-fail-provisional-load': ['errorCode', 'errorDescription', 'validatedURL', 'isMainFrame', 'currentURL'],
  'did-fail-load': ['errorCode', 'errorDescription', 'validatedURL', 'isMainFrame', 'ignoredByHandler'],
  'did-frame-finish-load': ['isMainFrame'],
  'did-start-loading': [],
  'did-stop-loading': [],
  'did-get-response-details': ['status', 'newURL', 'originalURL', 'httpResponseCode', 'requestMethod', 'referrer', 'headers', 'resourceType'],
  'did-get-redirect-request': ['oldURL', 'newURL', 'isMainFrame'],
  'dom-ready': [],
  'preferred-size-changed': ['size'],
  'console-message': ['level', 'message', 'line', 'sourceId'],
  'new-window': ['url', 'frameName', 'disposition', 'options'],
  'will-navigate': ['url'],
  'did-navigate': ['url', 'isRendererInitiated'],
  'did-navigate-in-page': ['url', 'isMainFrame'],
  'close': [],
  'crashed': [],
  'gpu-crashed': [],
  'plugin-crashed': ['name', 'version'],
  'will-destroy': [],
  'destroyed': [],
  'security-style-changed': ['securityState'],
  'page-favicon-updated': ['favicons'],
  'enter-html-full-screen': [],
  'leave-html-full-screen': [],
  'media-started-playing': [],
  'media-paused': [],
  'found-in-page': ['result'],
  'did-change-theme-color': ['themeColor'],
  'update-target-url': ['url', 'x', 'y'],
  'set-active': ['active'],
  'context-menu': ['params'],
  'enable-pepper-menu': ['params'],
  'repost-form-warning': [],
  'navigation-entry-commited': ['url', 'isInPage', 'didReplaceEntry'],
  'content-blocked': ['details'],
  'did-run-insecure-content': ['details'],
  'did-block-run-insecure-content': ['details'],
  'show-autofill-settings': [],
  'update-autofill-popup-data-list-values': ['values', 'labels'],
  'hide-autofill-popup': [],
  'show-autofill-popup': ['suggestions', 'rect']
}

var dispatchEvent = function (webView, eventName, eventKey, ...args) {
  var domEvent, f, i, j, len, ref1
  domEvent = new Event(eventName)
  ref1 = WEB_VIEW_EVENTS[eventKey]
  for (i = j = 0, len = ref1.length; j < len; i = ++j) {
    f = ref1[i]
    domEvent[f] = args[i]
  }
  webView.dispatchEvent(domEvent)
}

const GuestViewInternal = {
  registerEvents: function (webView, tabId) {
    ipcRenderer.on('ELECTRON_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-' + tabId, function (event, eventName, ...args) {
      dispatchEvent.apply(null, [webView, eventName, eventName].concat(args))
    })

    ipcRenderer.on('ELECTRON_GUEST_VIEW_INTERNAL_IPC_MESSAGE-' + tabId, function (event, channel, ...args) {
      var domEvent = new Event('ipc-message')
      domEvent.channel = channel
      domEvent.args = args
      webView.dispatchEvent(domEvent)
    })

  },
  deregisterEvents: function (tabId) {
    ipcRenderer.removeAllListeners('ELECTRON_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-' + tabId)
    ipcRenderer.removeAllListeners('ELECTRON_GUEST_VIEW_INTERNAL_IPC_MESSAGE-' + tabId)
  }
}

exports.$set('GuestViewInternal', GuestViewInternal);
