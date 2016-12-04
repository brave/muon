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

exports.on = ipcRenderer.on.bind(ipcRenderer);
exports.once = ipcRenderer.once.bind(ipcRenderer);
exports.send = ipcRenderer.send.bind(ipcRenderer);
exports.sendSync = ipcRenderer.sendSync.bind(ipcRenderer);
exports.sendToHost = ipcRenderer.sendToHost.bind(ipcRenderer);
exports.emit = ipcRenderer.emit.bind(ipcRenderer);
