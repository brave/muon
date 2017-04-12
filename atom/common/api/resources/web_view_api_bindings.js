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
  'reloadIgnoringCache',
  'clearHistory',
  'goBack',
  'goForward',
  'goToIndex',
  'goToOffset',
  'setUserAgent',
  'inspectElement',
  'setAudioMuted',
  'undo',
  'redo',
  'cut',
  'copy',
  'paste',
  'pasteAndMatchStyle',
  'delete',
  'selectAll',
  'unselect',
  'replace',
  'replaceMisspelling',
  'findInPage',
  'stopFindInPage',
  'downloadURL',
  'inspectServiceWorker',
  'print',
  'printToPDF',
  'showCertificate',
  'showDefinitionForSelection',
  'capturePage',
  'setTabIndex',
  'setWebRTCIPHandlingPolicy',
  'executeScriptInTab',
  'setZoomLevel',
  'zoomIn',
  'zoomOut',
  'zoomReset',
  'enablePreferredSizeMode',
  'close',
  'send',
  'getPreferredSize',
]

const syncMethods = [
  'getEntryCount',
  'getCurrentEntryIndex',
  'getURLAtIndex',
  'getTitleAtIndex',
  'getZoomPercent'
]

var WEB_VIEW_API_METHODS = [
  'attachGuest',
  'detachGuest',
  // Returns Chrome's internal process ID for the guest web page's current
  // process.
  'getProcessId',
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
    return
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
    this.guest.detach(() => {
      this.guest = new GuestView('webview')
    })
  }
}

WebViewImpl.prototype.getProcessId = function() {
  return this.processId
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
