// Copyright 2014 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This module implements the public-facing API functions for the <webview> tag.

const GuestView = require('guestView').GuestView;

GuestView.prototype.getState = function() {
  var internal = privates(this).internal;
  return internal.state
}
