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
WebViewImpl.prototype.attachWindow$ = function (opt_guestInstanceId) {
  if (this.guest.getId() === opt_guestInstanceId &&
      this.guest.getState() === GuestViewImpl.GuestState.GUEST_STATE_ATTACHED) {
    return true
  }
  const guestInstanceId = opt_guestInstanceId || this.guest.getId()

  if (opt_guestInstanceId) {
    if (this.guest.getState() === GuestViewImpl.GuestState.GUEST_STATE_ATTACHED) {
      this.guest.detach()
    }

    this.guest = new GuestView('webview', guestInstanceId)
  }

  const attached = GuestViewContainer.prototype.attachWindow$.call(this)

  if (attached) {
    WebViewInternal.getWebContents(guestInstanceId, (webContents) => {
      // cache webContents_
      this.webContents_ = webContents
    })
  }
  return attached
}

WebViewImpl.prototype.detachGuest = function () {
  if (this.guest.getState() === GuestViewImpl.GuestState.GUEST_STATE_ATTACHED) {
    this.guest.detach()
  }
}

WebViewImpl.prototype.attachGuest = function (guestInstanceId) {
  if (this.guest.getState() === GuestViewImpl.GuestState.GUEST_STATE_ATTACHED) {
    this.guest.detach(() => {
      return this.attachWindow$(guestInstanceId)
    })
  } else {
    return this.attachWindow$(guestInstanceId)
  }
}

WebViewImpl.prototype.setTabId = function (tabID) {
  this.tabID = tabID
  GuestViewInternal.registerEvents(this, tabID)
}

WebViewImpl.prototype.getId = function () {
  return this.tabID
}

WebViewImpl.prototype.getURL = function () {
  return this.attributes[WebViewConstants.ATTRIBUTE_SRC]
}
// -----------------------------------------------------------------------------

WebViewImpl.getApiMethods = function () {
  return WEB_VIEW_API_METHODS
}
