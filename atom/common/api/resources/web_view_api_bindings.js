// Copyright 2014 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This module implements the public-facing API functions for the <webview> tag.

const GuestViewInternal = require('guest-view-internal').GuestViewInternal
const TabViewInternal = require('tabViewInternal').TabViewInternal;
const WebViewInternal = require('webViewInternal').WebViewInternal;
const WebViewImpl = require('webView').WebViewImpl;
const GuestViewImpl = require('guestView').GuestViewImpl
const remote = require('remote')
const GuestViewContainer = require('guestViewContainer').GuestViewContainer;
const GuestView = require('guestView').GuestView;

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
  'attachGuest',
  'detachGuest',
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
  if (this.guest.getId() === opt_guestInstanceId && this.guest.getState() === GuestViewImpl.GuestState.GUEST_STATE_ATTACHED) {
    console.log('ID is the same so bailing', opt_guestInstanceId)
    return
  }
  // preload the webcontents and tabID
  const guestInstanceId = opt_guestInstanceId || this.guest.getId()

  console.log('web_view_api_bindings.js attachGuest!', guestInstanceId, 'guest state:', this.guest.getState())
  // TODO: Should be destroying this not detaching
  if (this.guest.getState() === GuestViewImpl.GuestState.GUEST_STATE_ATTACHED) {
    console.log('calling detach!')
    this.guest.detach();
  }
  console.log('creating new GuestView!')
  this.guest = new GuestView('webview', guestInstanceId);

  const attached = GuestViewContainer.prototype.attachWindow$.call(this);

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

WebViewImpl.prototype.detachGuest = function () {
  console.log('detachGuest1', this.guest.getState())
  if (this.guest.getState() === GuestViewImpl.GuestState.GUEST_STATE_ATTACHED) {
    console.log('detachGuest2')
    this.guest.detach()
  }
  this.guest = new GuestView('webview')
  console.log('detachGuest3')
  // TODO: Can try: this.createGuest()
}

WebViewImpl.prototype.getProcessId = function() {
  return this.processId
}

WebViewImpl.prototype.attachGuest = function(guestInstanceId) {

  return this.attachWindow$(guestInstanceId)

};

GuestView.prototype.getState = function() {
  var internal = privates(this).internal;
  return internal.state
}


// -----------------------------------------------------------------------------

WebViewImpl.getApiMethods = function() {
  return WEB_VIEW_API_METHODS;
};
