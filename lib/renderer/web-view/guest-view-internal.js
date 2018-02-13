'use strict'

const ipcRenderer = require('ipc_utils')
const EventEmitter = require('event_emitter').EventEmitter

var WEB_VIEW_EVENTS = {
  'load-start': ['url', 'isMainFrame', 'isErrorPage'],
  'will-detach': [],
  'did-attach': ['tabId'],
  'guest-ready': ['tabId', 'guestInstanceId'],
  'did-detach': [],
  'did-finish-load': ['validatedURL'],
  'did-fail-provisional-load': ['errorCode', 'errorDescription', 'validatedURL', 'isMainFrame', 'currentURL'],
  'did-fail-load': ['errorCode', 'errorDescription', 'validatedURL', 'isMainFrame'],
  'dom-ready': [],
  'preferred-size-changed': ['size'],
  'console-message': ['level', 'message', 'line', 'sourceId'],
  'did-navigate': ['url', 'isRendererInitiated'],
  'did-navigate-in-page': ['url', 'isMainFrame'],
  'close': [],
  'gpu-crashed': [],
  'plugin-crashed': ['name', 'version'],
  'will-destroy': [],
  'destroyed': [],
  'security-style-changed': ['securityState', 'securityInfo'],
  'page-favicon-updated': ['favicons'],
  'enter-html-full-screen': [],
  'leave-html-full-screen': [],
  'media-started-playing': [],
  'media-paused': [],
  'found-in-page': ['result'],
  'did-change-theme-color': ['themeColor'],
  'update-target-url': ['url', 'x', 'y'],
  'context-menu': ['params'],
  'enable-pepper-menu': ['params'],
  'repost-form-warning': [],
  'content-blocked': ['details'],
  'did-run-insecure-content': ['details'],
  'did-block-run-insecure-content': ['details'],
  'show-autofill-settings': [],
  'update-autofill-popup-data-list-values': ['values', 'labels'],
  'hide-autofill-popup': [],
  'show-autofill-popup': ['suggestions', 'rect']
}

const GuestViewInternal = new EventEmitter

var dispatchEvent = function (tabId, eventName, ...args) {
  var domEvent, f, i, j, len, ref1
  domEvent = new Event(eventName)
  ref1 = WEB_VIEW_EVENTS[eventName]
  for (i = j = 0, len = ref1.length; j < len; i = ++j) {
    f = ref1[i]
    domEvent[f] = args[i]
  }

  GuestViewInternal.emit(tabId, domEvent)
}

ipcRenderer.on('ELECTRON_GUEST_VIEW_INTERNAL_DISPATCH_EVENT', function (event, tabId, eventName, ...args) {
  dispatchEvent.apply(null, [tabId, eventName].concat(args))
})

ipcRenderer.on('ELECTRON_GUEST_VIEW_INTERNAL_IPC_MESSAGE', function (event, tabId, channel, ...args) {
  var domEvent = new Event('ipc-message')
  domEvent.channel = channel
  domEvent.args = args
  GuestViewInternal.emit(tabId, domEvent)
})

exports.$set('GuestViewInternal', GuestViewInternal);
