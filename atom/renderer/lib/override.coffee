{ipcRenderer, remote} = require 'electron'

# Helper function to resolve relative url.
a = window.top.document.createElement 'a'
resolveURL = (url) ->
  a.href = url
  a.href

# Window object returned by "window.open".
class BrowserWindowProxy
  @proxies: {}

  @getOrCreate: (guestId) ->
    @proxies[guestId] ?= new BrowserWindowProxy(guestId)

  @remove: (guestId) ->
    delete @proxies[guestId]

  constructor: (@guestId) ->
    @closed = false
    ipcRenderer.once "ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_CLOSED_#{@guestId}", =>
      BrowserWindowProxy.remove(@guestId)
      @closed = true

  close: ->
    return if @closed
    ipcRenderer.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_CLOSE', @guestId

  focus: ->
    return if @closed
    ipcRenderer.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_METHOD', @guestId, 'focus'

  blur: ->
    return if @closed
    ipcRenderer.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_METHOD', @guestId, 'blur'

  postMessage: (message, targetOrigin='*') ->
    return if @closed
    ipcRenderer.send 'ATOM_SHELL_GUEST_WINDOW_MANAGER_WINDOW_POSTMESSAGE', @guestId, message, targetOrigin, location.origin

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

if process.openerId?
  window.opener = BrowserWindowProxy.getOrCreate process.openerId

ipcRenderer.on 'ATOM_SHELL_GUEST_WINDOW_POSTMESSAGE', (event, sourceId, message, sourceOrigin) ->
  # Manually dispatch event instead of using postMessage because we also need to
  # set event.source.
  event = document.createEvent 'Event'
  event.initEvent 'message', false, false
  event.data = message
  event.origin = sourceOrigin
  event.source = BrowserWindowProxy.getOrCreate(sourceId)
  window.dispatchEvent event

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
