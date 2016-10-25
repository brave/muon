var ipc = require('ipc_utils')

exports.didCreateDocumentElement = function() {
  // don't try to run if there is no window object
  if (!window)
    return

  window.alert = function(message, title) {
    return ipc.send('window-alert', message, title)
  }

  window.confirm = function(message, title) {
    return ipc.sendSync('window-confirm', message, title)
  }

  window.prompt = function(text, defaultText) {
    return ipc.sendSync('window-prompt', text, defaultText)
  }

  if (window.chrome && window.chrome.tabs && window.chrome.tabs.create)
    window.open = function(url, name, specs, replace) {
      if (name !== undefined) {
        throw new Error('name of window.open is not supported yet')
      }
      if (specs !== undefined) {
        throw new Error('specs of window.open is not supported yet')
      }
      if (replace !== undefined) {
        throw new Error('replace of window.open is not supported yet')
      }
      return window.chrome.tabs.create({url: url})
    }
}

exports.binding = ipc
