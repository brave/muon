var runtime = require('runtime').binding
var tabs = require('tabs').binding

runtime.openOptionsPage = function (cb) {
  let manifest = runtime.getManifest()
  let optionsPageURL = runtime.getURL(manifest.options_page)
  tabs.create({url: optionsPageURL})
  cb && cb()
}
