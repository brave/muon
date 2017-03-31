// Copyright 2014 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This module implements the public-facing API functions for the <webview> tag.

const WebViewEvents = require('webViewEvents').WebViewEvents;
const CreateEvent = require('guestViewEvents').CreateEvent;

WebViewEvents.EVENTS['detachguest'] = {
  evt: CreateEvent('webViewInternal.onDetachGuest'),
  fields: [],
  handler: 'handleDetachGuest',
}

WebViewEvents.EVENTS['attachguest'] = {
  evt: CreateEvent('webViewInternal.onAttachGuest'),
  fields: ['guestInstanceId'],
  handler: 'handleAttachGuest'
}

WebViewEvents.EVENTS['tab-id-changed'] = {
  evt: CreateEvent('webViewInternal.onTabIdChanged'),
  fields: ['tabID'],
  handler: 'handleTabIdChanged'
}

WebViewEvents.prototype.handleDetachGuest = function (event, eventName) {
  this.view.detachGuest()
}

WebViewEvents.prototype.handleAttachGuest = function (event, eventName) {
  this.view.attachGuest(event.guestInstanceId)
}

WebViewEvents.prototype.handleTabIdChanged = function (event, eventName) {
  this.view.setTabId(event.tabID)
  const webViewEvent = this.makeDomEvent(event, eventName)
  this.view.dispatchEvent(webViewEvent)
}
