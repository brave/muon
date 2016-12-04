var atom = requireNative('atom').GetBinding()
var EventEmitter = require('event_emitter').EventEmitter
var ipc = atom.ipc

var ipcRenderer = atom.v8.getHiddenValue('ipc')
if (!ipcRenderer) {
  ipcRenderer = new EventEmitter

  ipcRenderer.send = function () {
    var args
    args = 1 <= arguments.length ? $Array.slice(arguments, 0) : []
    return ipc.send('ipc-message', $Array.slice(args))
  }

  ipcRenderer.sendSync = function () {
    var args
    args = 1 <= arguments.length ? $Array.slice(arguments, 0) : []
    return $JSON.parse(ipc.sendSync('ipc-message-sync', $Array.slice(args)))
  }

  ipcRenderer.sendToHost = function () {
    var args
    args = 1 <= arguments.length ? $Array.slice(arguments, 0) : []
    return ipc.send('ipc-message-host', $Array.slice(args))
  }

  ipcRenderer.emit = function () {
    arguments[1].sender = ipcRenderer
    return $Function.apply(EventEmitter.prototype.emit, ipcRenderer, arguments)
  }
  atom.v8.setHiddenValue('ipc', ipcRenderer)
}

function guid() {
  function s4() {
    return Math.floor((1 + Math.random()) * 0x10000)
      .toString(16)
      .substring(1);
  }
  return s4() + s4() + '-' + s4() + '-' + s4() + '-' +
    s4() + '-' + s4() + s4() + s4();
}

exports.$set('guid', guid)
exports.$set('removeListener', ipcRenderer.removeListener.bind(ipcRenderer))
exports.$set('off', ipcRenderer.off.bind(ipcRenderer))
exports.$set('removeAllListeners', ipcRenderer.removeAllListeners.bind(ipcRenderer))
exports.$set('on', ipcRenderer.on.bind(ipcRenderer))
exports.$set('once', ipcRenderer.once.bind(ipcRenderer))
exports.$set('send', ipcRenderer.send.bind(ipcRenderer))
exports.$set('sendSync', ipcRenderer.sendSync.bind(ipcRenderer))
exports.$set('sendToHost', ipcRenderer.sendToHost.bind(ipcRenderer))
exports.$set('emit', ipcRenderer.emit.bind(ipcRenderer))

