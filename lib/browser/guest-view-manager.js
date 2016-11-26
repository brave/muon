'use strict'

const ipcMain = require('electron').ipcMain
const webContents = require('electron').webContents

let supportedWebViewEvents = [
  'load-start',
  'load-commit',
  'did-attach',
  'did-detach',
  'did-finish-load',
  'did-fail-provisional-load',
  'did-fail-load',
  'did-frame-finish-load',
  'did-start-loading',
  'did-stop-loading',
  'did-get-response-details',
  'did-get-redirect-request',
  'document-available',
  'document-onload',
  'dom-ready',
  'preferred-size-changed',
  'console-message',
  'devtools-opened',
  'devtools-closed',
  'devtools-focused',
  'new-window',
  'will-navigate',
  'did-navigate',
  'did-navigate-in-page',
  'security-style-changed',
  'close',
  'crashed',
  'gpu-crashed',
  'plugin-crashed',
  'will-destroy',
  'destroyed',
  'page-title-updated',
  'page-favicon-updated',
  'enter-html-full-screen',
  'leave-html-full-screen',
  'media-started-playing',
  'media-paused',
  'found-in-page',
  'did-change-theme-color',
  'update-target-url',
  'load-progress-changed',
  'set-active',
  'context-menu',
  'repost-form-warning',
  'navigation-entry-commited',
  'content-blocked',
  'did-display-insecure-content',
  'show-autofill-settings',
  'update-autofill-popup-data-list-values',
  'hide-autofill-popup',
  'show-autofill-popup',
  'did-display-insecure-content',
  'did-run-insecure-content',
  'did-block-display-insecure-content',
  'did-block-run-insecure-content'
]

let guests = {}
const registerGuest = function (guest, tabId) {
  if (guests[tabId])
    return

  guests[tabId] = true

  // Dispatch events to embedder.
  guest.once('will-destroy', function () {
    delete guests[tabId]
  })
  guest.once('destroyed', function () {
    delete guests[tabId]
  })
  guest.once('close', function () {
    delete guests[tabId]
  })
  guest.once('crashed', function () {
    delete guests[tabId]
  })

  const fn = function (event) {
    guest.on(event, function (_, ...args) {
      if (guest.isDestroyed() || !guests[tabId])
        return

      const embedder = guest.hostWebContents
      if (embedder) {
        embedder.send.apply(embedder, ['ELECTRON_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-' + tabId, event].concat(args))
      }
    })
  }
  for (const event of supportedWebViewEvents) {
    fn(event)
  }

  // Dispatch guest's IPC messages to embedder.
  guest.on('ipc-message-host', function (_, [channel, ...args]) {
    if (guest.isDestroyed() || !guests[tabId])
      return

    const embedder = guest.hostWebContents
    if (embedder) {
      embedder.send.apply(embedder, ['ELECTRON_GUEST_VIEW_INTERNAL_IPC_MESSAGE-' + tabId, channel].concat(args))
    }
  })
}

exports.registerGuest = registerGuest
