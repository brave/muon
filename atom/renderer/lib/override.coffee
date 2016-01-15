{ipcRenderer, remote} = require 'electron'

unless process.guestInstanceId?
  # Override default window.close.
  window.close = ->
    remote.getCurrentWindow().close()

# Use the dialog API to implement alert().
window.alert = (message, title='') ->
  buttons = ['OK']
  message = message.toString()
  remote.dialog.showMessageBox remote.getCurrentWindow(), {message, title, buttons}
  # Alert should always return undefined.
  return

# And the confirm().
window.confirm = (message, title='') ->
  buttons = ['OK', 'Cancel']
  cancelId = 1
  not remote.dialog.showMessageBox remote.getCurrentWindow(), {message, title, buttons, cancelId}

# But we do not support prompt().
window.prompt = ->
  throw new Error('prompt() is and will not be supported.')

# Forward history operations to browser.
sendHistoryOperation = (args...) ->
  ipcRenderer.send 'ATOM_SHELL_NAVIGATION_CONTROLLER', args...

getHistoryOperation = (args...) ->
  ipcRenderer.sendSync 'ATOM_SHELL_SYNC_NAVIGATION_CONTROLLER', args...

window.history.back = -> sendHistoryOperation 'goBack'
window.history.forward = -> sendHistoryOperation 'goForward'
window.history.go = (offset) -> sendHistoryOperation 'goToOffset', offset
Object.defineProperty window.history, 'length',
  get: ->
    getHistoryOperation 'length'

# Make document.hidden and document.visibilityState return the correct value.
Object.defineProperty document, 'hidden',
  get: ->
    currentWindow = remote.getCurrentWindow()
    currentWindow.isMinimized() || !currentWindow.isVisible()

Object.defineProperty document, 'visibilityState',
  get: ->
    if document.hidden then "hidden" else "visible"
