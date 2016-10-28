var ipc = require('ipc_utils');
var process = requireNative('process');
var extensionId = process.GetExtensionId();

var binding = {
  removeAll: function () {
    console.warn('chrome.contentMenus.removeAll is not supported yet')
  },
  create: function (properties, cb) {
    console.warn('chrome.contentMenus.create is not supported yet')
    cb && cb()
  }
}

exports.binding = binding;

// TODO (Anthony): Move this to separated binding
var runtime = require('runtime').binding
var tabs = require('tabs').binding

runtime.openOptionsPage = function (cb) {
  let manifest = runtime.getManifest()
  let optionsPageURL = runtime.getURL(manifest.options_page)
  tabs.create({url: optionsPageURL})
  cb && cb()
}
