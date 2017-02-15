// Copyright 2014 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This module implements the public-facing API functions for the <webview> tag.

const GuestViewInternal = require('guest-view-internal').GuestViewInternal
const TabViewInternal = require('tabViewInternal').TabViewInternal;
const WebViewInternal = require('webViewInternal').WebViewInternal;
const WebViewImpl = require('webView').WebViewImpl;
const remote = require('remote')

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
  'setActive',
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
  'getId',
  'isFocused',
  'getZoomPercent',
  'getURL',
]

var WEB_VIEW_API_METHODS = [
  'setGuestInstanceId',
  // Returns Chrome's internal process ID for the guest web page's current
  // process.
  'getProcessId',
].concat(asyncMethods).concat(syncMethods)

asyncMethods.forEach((method) => {
  WebViewImpl.prototype[method] = function () {
    if (!this.guest.getId())
      return

    this.getTabID(this.guest.getId(), (tabID) => {
      remote.callAsyncWebContentsFunction(tabID, method, arguments)
    })
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
WebViewImpl.prototype.getTabID = function (instanceId, cb) {
  if (!this.tabID) {
    TabViewInternal.getTabID(instanceId, (tabID) => {
      this.tabID = tabID
      cb(tabID)
    })
  } else {
    cb(this.tabID)
  }
}

const attachWindow = WebViewImpl.prototype.attachWindow$
WebViewImpl.prototype.attachWindow$ = function(opt_guestInstanceId) {
  let attached = attachWindow.bind(this)(opt_guestInstanceId)
  // preload the webcontents and tabID
  const guestInstanceId = opt_guestInstanceId || this.guest.getId()

  WebViewInternal.getWebContents(guestInstanceId, (webContents) => {
    // cache webContents_
    this.webContents_ = webContents
  })
  this.getTabID(guestInstanceId, (tabID) => {
    // cache tabId
    this.tabID = tabID
    GuestViewInternal.registerEvents(this, tabID)
  })

  return attached
}

WebViewImpl.prototype.setGuestInstanceId = function (guestInstanceId) {
  return this.attachWindow$(guestInstanceId)
}

WebViewImpl.prototype.getProcessId = function() {
  return this.processId
}

// -----------------------------------------------------------------------------

WebViewImpl.getApiMethods = function() {
  return WEB_VIEW_API_METHODS;
};
