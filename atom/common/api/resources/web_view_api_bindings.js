// Copyright 2014 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This module implements the public-facing API functions for the <webview> tag.

const GuestViewInternal = require('guest-view-internal').GuestViewInternal
const TabViewInternal = require('tabViewInternal').TabViewInternal
const WebViewInternal = require('webViewInternal').WebViewInternal
const WebViewImpl = require('webView').WebViewImpl
const GuestViewImpl = require('guestView').GuestViewImpl
const remote = require('remote')
const GuestViewContainer = require('guestViewContainer').GuestViewContainer
const GuestView = require('guestView').GuestView

const asyncMethods = [
  'loadURL',
  'stop',
  'reload',
  'undo',
  'redo',
  'cut',
  'copy',
  'pasteAndMatchStyle',
  'findInPage',
  'stopFindInPage',
  'downloadURL',
  'print',
  'showCertificate',
  'showDefinitionForSelection',
  'executeScriptInTab',
  'zoomIn',
  'zoomOut',
  'zoomReset',
  'enablePreferredSizeMode',
  'send',
  'getPreferredSize',
]

const syncMethods = [
  'getZoomPercent'
]

var WEB_VIEW_API_METHODS = [
  'attachGuest',
  'detachGuest',
  'getId',
  'getGuestId'
].concat(asyncMethods).concat(syncMethods)

asyncMethods.forEach((method) => {
  WebViewImpl.prototype[method] = function () {
    if (!this.tabID)
      return

    remote.callAsyncWebContentsFunction(this.tabID, method, arguments)
  }
})

syncMethods.forEach((method) => {
  WebViewImpl.prototype[method] = function () {
    if (!this.guest.getId())
      return

    if (!this.webContents_) {
      console.error('webContents is not available for: ' + method)
    } else {
      return this.webContents_[method].apply(this, arguments)
    }
  }
})

// -----------------------------------------------------------------------------
// Custom API method implementations.
const attachWindow = WebViewImpl.prototype.attachWindow$
WebViewImpl.prototype.attachWindow$ = function (opt_guestInstanceId, webContents) {
  if (this.guest.getId() === opt_guestInstanceId &&
      this.guest.getState() === GuestViewImpl.GuestState.GUEST_STATE_ATTACHED) {
    return true
  }
  const guestInstanceId = opt_guestInstanceId || this.guest.getId()

  if (opt_guestInstanceId) {
    // detach if attached so that attached contents is not destroyed
    // but never try to call detach on a destroyed contents guest,
    // as that request will never be fulfilled.
    this.detachGuest() // no need to await detaching since we create a new guest
    this.guest = new GuestView('webview', guestInstanceId)
  }

  const attached = GuestViewContainer.prototype.attachWindow$.call(this)

  if (attached) {
    // cache webContents_
    if (webContents) {
      this.webContents_ = webContents
    } else {
      WebViewInternal.getWebContents(guestInstanceId, (webContents) => {
        this.webContents_ = webContents
      })
    }
  }
  return attached
}

WebViewImpl.prototype.detachGuest = function () {
  // do not attempt to call detach on a destroyed web contents
  if (
    !this.webContents_ ||
    this.webContents_.isDestroyed() ||
    !this.guest ||
    this.guest.guestIsDetaching_ ||
    this.guest.getState() !== GuestViewImpl.GuestState.GUEST_STATE_ATTACHED
  ) {
    return Promise.resolve()
  }
  this.guest.guestIsDetaching_ = true
  // perform detach
  // resolve when detached or destroyed
  return new Promise(resolve => {
    // guest may destroy or detach
    if (this.webContents_) {
      this.webContents_.addListener('will-destroy', resolve)
    }
    this.guest.detach(() => {
      this.webContents_.removeListener('will-destroy', resolve)
      // don't forward tab events to this webview anymore
      if (this.tabID && this.currentEventWrapper) {
        GuestViewInternal.removeListener(this.tabID, this.currentEventWrapper)
      }
      this.guest.guestIsDetaching_ = false
      resolve()
    })
  })
}

WebViewImpl.prototype.attachGuest = function (guestInstanceId, webContents) {
    return this.attachWindow$(guestInstanceId, webContents)
}

WebViewImpl.prototype.eventWrapper = function (tabID, event) {
  if (event.type == 'destroyed') {
    GuestViewInternal.removeListener(tabID, this.currentEventWrapper)
  }
  this.dispatchEvent(event)
}

WebViewImpl.prototype.setTabId = function (tabID) {
  if (this.tabID) {
    GuestViewInternal.removeListener(this.tabID, this.currentEventWrapper)
  }
  this.tabID = tabID
  this.currentEventWrapper = this.eventWrapper.bind(this, tabID)
  GuestViewInternal.addListener(tabID, this.currentEventWrapper)
}

WebViewImpl.prototype.getId = function () {
  return this.tabID
}

WebViewImpl.prototype.getGuestId = function () {
  return this.guest.getId()
}

WebViewImpl.prototype.getURL = function () {
  return this.attributes[WebViewConstants.ATTRIBUTE_SRC]
}
// -----------------------------------------------------------------------------

WebViewImpl.getApiMethods = function () {
  return WEB_VIEW_API_METHODS
}
