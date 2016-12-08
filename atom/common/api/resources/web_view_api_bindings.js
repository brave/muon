// Copyright 2014 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This module implements the public-facing API functions for the <webview> tag.

const WebViewInternal = require('webViewInternal').WebViewInternal;
const TabViewInternal = require('tabViewInternal').TabViewInternal;
const WebViewImpl = require('webView').WebViewImpl;
const remote = require('remote')
const webFrameNatives = requireNative('webFrame')
const GuestViewInternal = require('guest-view-internal').GuestViewInternal

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
  'openDevTools',
  'closeDevTools',
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
  'showDefinitionForSelection',
  'capturePage',
  'setActive',
  'setWebRTCIPHandlingPolicy',
  'executeScriptInTab',
  'setZoomLevel',
  'zoomIn',
  'zoomOut',
  'zoomReset',
  'enablePreferredSizeMode',
  'close',
  'focus',
  'send',
]

const syncMethods = [
  'getZoomPercent',
  'getURL',
  'canGoBack',
  'canGoForward',
  'getWebRTCIPHandlingPolicy',
  'getWebContents',
  'isDevToolsOpened',
  'getPreferredSize',
]

var WEB_VIEW_API_METHODS = [
  'setGuestInstanceId',
  // Returns Chrome's internal process ID for the guest web page's current
  // process.
  'getProcessId',

  // Returns the user agent string used by the webview for guest page requests.
  'getUserAgent',

  // Gets the current zoom factor.
  'getZoom',

  // Gets the current zoom mode of the webview.
  'getZoomMode',

  // Indicates whether or not the webview's user agent string has been
  // overridden.
  'isUserAgentOverridden',

  // Override the user agent string used by the webview for guest page requests.
  'setUserAgentOverride',

  // Changes the zoom factor of the page.
  'setZoom',

  'executeJavaScript',
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

    let webContents = this.getWebContents()
    if (!webContents)
      throw 'webContents is not available'
    return webContents[method].apply(this, arguments)
  }
})

// GuestViewContainer.prototype.weakWrapper = function(func) {
//   var viewInstanceId = this.viewInstanceId;
//   return function() {
//     var view = GuestViewInternalNatives.GetViewFromID(viewInstanceId);
//     if (view) {
//       return $Function.apply(func, view, $Array.slice(arguments));
//     }
//   };
// };

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

WebViewImpl.prototype.executeJavaScript = function (code) {
  return webFrameNatives.executeJavaScript(code)
}

const attachWindow = WebViewImpl.prototype.attachWindow$
WebViewImpl.prototype.attachWindow$ = function(opt_guestInstanceId) {
  let attached = attachWindow.bind(this)(opt_guestInstanceId)
  // preload the webcontents and tabID
  const guestInstanceId = opt_guestInstanceId || this.guest.getId()
  this.getTabID(guestInstanceId, (tabID) => {
    this.tabID = tabID
    GuestViewInternal.registerEvents(this, tabID)
  })
  this.getWebContents()
  return attached
}

WebViewImpl.prototype.setGuestInstanceId = function (guestInstanceId) {
  return this.attachWindow$(guestInstanceId)
}

WebViewImpl.prototype.getWebContents = function (cb) {
  if (!this.webContents_) {
    WebViewInternal.getWebContents(this.guest.getId(), (webContents) => {
      this.webContents_ = webContents
      cb && cb(this.webContents_)
    })
  } else {
    cb && cb(this.webContents_)
  }
  return this.webContents_
}

WebViewImpl.prototype.getProcessId = function() {
  return this.processId;
};

WebViewImpl.prototype.getUserAgent = function() {
  return this.userAgentOverride || navigator.userAgent;
};

WebViewImpl.prototype.isUserAgentOverridden = function() {
  return !!this.userAgentOverride &&
      this.userAgentOverride != navigator.userAgent;
};

WebViewImpl.prototype.setUserAgentOverride = function(userAgentOverride) {
  this.userAgentOverride = userAgentOverride;
  if (!this.guest.getId()) {
    // If we are not attached yet, then we will pick up the user agent on
    // attachment.
    return false;
  }
  // TODO(bridiver) - FIX THIS!!!
  // WebViewInternal.overrideUserAgent(this.guest.getId(), userAgentOverride);
  return true;
};

WebViewImpl.prototype.setZoom = function(zoomFactor, callback) {
  if (!this.guest.getId()) {
    this.cachedZoomFactor = zoomFactor;
    return false;
  }
  this.cachedZoomFactor = 1;
  WebViewInternal.setZoom(this.guest.getId(), zoomFactor, callback);
  return true;
};

// -----------------------------------------------------------------------------

WebViewImpl.getApiMethods = function() {
  return WEB_VIEW_API_METHODS;
};

// WebViewImpl.setupElement(WebViewImpl.prototype)
