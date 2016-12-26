'use strict'

const ipcMain = require('electron').ipcMain
const webContents = require('electron').webContents

let supportedWebViewEvents = [
  'load-start',
  'load-commit',
  'did-attach',
  'guest-ready',
  'did-detach',
  'did-finish-load',
  'did-fail-provisional-load',
  'did-fail-load',
  'did-frame-finish-load',
  'did-start-loading',
  'did-stop-loading',
  'did-get-response-details',
  'did-get-redirect-request',
  'dom-ready',
  'preferred-size-changed',
  'console-message',
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
  'page-favicon-updated',
  'enter-html-full-screen',
  'leave-html-full-screen',
  'media-started-playing',
  'media-paused',
  'found-in-page',
  'did-change-theme-color',
  'update-target-url',
  'set-active',
  'context-menu',
  'enable-pepper-menu',
  'repost-form-warning',
  'navigation-entry-commited',
  'content-blocked',
  'show-autofill-settings',
  'update-autofill-popup-data-list-values',
  'hide-autofill-popup',
  'show-autofill-popup',
  'did-run-insecure-content',
  'did-block-run-insecure-content'
]

let guests = {}
const registerGuest = function (guest, embedder) {
  const tabId = guest.getId()

  embedder.once('destroyed', function () {
    if (guests[tabId] === embedder) {
      guests[tabId] = false
    }
  })

  const oldEmbedder = guests[tabId]
  guests[tabId] = embedder
  if (oldEmbedder !== undefined) {
    return
  }

  guest.once('destroyed', function () {
    delete guests[tabId]
  })
  guest.once('crashed', function () {
    delete guests[tabId]
  })

  // Dispatch events to embedder.
  const fn = function (event) {
    guest.on(event, function (_, ...args) {
      const embedder = guests[tabId]
      if (!embedder || embedder.isDestroyed() || guest.isDestroyed())
        return

      embedder.send.apply(embedder, ['ELECTRON_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-' + tabId, event].concat(args))
    })
  }
  for (const event of supportedWebViewEvents) {
    fn(event)
  }

  // Dispatch guest's IPC messages to embedder.
  guest.on('ipc-message-host', function (_, [channel, ...args]) {
    const embedder = guests[tabId]
    if (!embedder || embedder.isDestroyed() || guest.isDestroyed())
      return

    embedder.send.apply(embedder, ['ELECTRON_GUEST_VIEW_INTERNAL_IPC_MESSAGE-' + tabId, channel].concat(args))
  })
}

exports.registerGuest = registerGuest
