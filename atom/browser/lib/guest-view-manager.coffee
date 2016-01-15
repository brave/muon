{ipcMain, webContents} = require 'electron'

webViewManager = null  # Doesn't exist in early initialization.

supportedWebViewEvents = [
  'load-commit'
  'did-finish-load'
  'did-fail-load'
  'did-frame-finish-load'
  'did-start-loading'
  'did-stop-loading'
  'did-get-response-details'
  'did-get-redirect-request'
  'dom-ready'
  'console-message'
  'devtools-opened'
  'devtools-closed'
  'devtools-focused'
  'new-window'
  'will-navigate'
  'did-navigate'
  'did-navigate-in-page'
  'close'
  'crashed'
  'gpu-crashed'
  'plugin-crashed'
  'destroyed'
  'page-title-updated'
  'page-favicon-updated'
  'enter-html-full-screen'
  'leave-html-full-screen'
  'media-started-playing'
  'media-paused'
  'found-in-page'
  'did-change-theme-color'
]

nextInstanceId = 0
guestInstances = {}
embedderElementsMap = {}
reverseEmbedderElementsMap = {}

# Moves the last element of array to the first one.
moveLastToFirst = (list) ->
  list.unshift list.pop()

# Generate guestInstanceId.
getNextInstanceId = (webContents) ->
  ++nextInstanceId

createWebContents = (embedder, params) ->
  webContents.create {isGuest: true, partition: params.partition, embedder}

# Create a new guest instance.
addGuest = (embedder, webContents, guestInstanceId) ->
  webViewManager ?= process.atomBinding 'web_view_manager'

  id = guestInstanceId or getNextInstanceId(embedder)
  guest = webContents
  guestInstances[id] = {guest, embedder}

  # Destroy guest when the embedder is gone or navigated.
  destroyEvents = ['destroyed', 'crashed', 'did-navigate']
  destroy = ->
    destroyGuest embedder, id if guestInstances[id]?
  for event in destroyEvents
    embedder.once event, destroy
    # Users might also listen to the crashed event, so We must ensure the guest
    # is destroyed before users' listener gets called. It is done by moving our
    # listener to the first one in queue.
    listeners = embedder._events[event]
    moveLastToFirst listeners if Array.isArray listeners
  guest.once 'destroyed', ->
    embedder.removeListener event, destroy for event in destroyEvents

  # Init guest web view after attached.
  guest.once 'did-attach', ->
    params = @attachParams
    delete @attachParams

    @viewInstanceId = params.instanceId
    @setSize
      normal:
        width: params.elementWidth, height: params.elementHeight
      enableAutoSize: params.autosize
      min:
        width: params.minwidth, height: params.minheight
      max:
        width: params.maxwidth, height: params.maxheight

    if params.src
      opts = {}
      opts.httpReferrer = params.httpreferrer if params.httpreferrer
      opts.userAgent = params.useragent if params.useragent
      @loadURL params.src, opts

    if params.allowtransparency?
      @setAllowTransparency params.allowtransparency

    guest.allowPopups = params.allowpopups

  # Dispatch events to embedder.
  for event in supportedWebViewEvents
    do (event) ->
      guest.on event, (_, args...) ->
        embedder.send "ATOM_SHELL_GUEST_VIEW_INTERNAL_DISPATCH_EVENT-#{guest.viewInstanceId}", event, args...

  # Dispatch guest's IPC messages to embedder.
  guest.on 'ipc-message-host', (_, packed) ->
    [channel, args...] = packed
    embedder.send "ATOM_SHELL_GUEST_VIEW_INTERNAL_IPC_MESSAGE-#{guest.viewInstanceId}", channel, args...

  # Autosize.
  guest.on 'size-changed', (_, args...) ->
    embedder.send "ATOM_SHELL_GUEST_VIEW_INTERNAL_SIZE_CHANGED-#{guest.viewInstanceId}", args...

  id

# Attach the guest to an element of embedder.
attachGuest = (embedder, elementInstanceId, guestInstanceId, params) ->
  guest = guestInstances[guestInstanceId].guest

  # Destroy the old guest when attaching.
  key = "#{embedder.getId()}-#{elementInstanceId}"
  oldGuestInstanceId = embedderElementsMap[key]
  if oldGuestInstanceId?
    # Reattachment to the same guest is not currently supported.
    return unless oldGuestInstanceId != guestInstanceId

    return unless guestInstances[oldGuestInstanceId]?
    destroyGuest embedder, oldGuestInstanceId

  webPreferences =
    guestInstanceId: guestInstanceId
    nodeIntegration: params.nodeintegration ? false
    plugins: params.plugins
    webSecurity: !params.disablewebsecurity
  webPreferences.preloadURL = params.preload if params.preload

  webViewManager.addGuest guestInstanceId, elementInstanceId, embedder, guest, webPreferences
  params.guestInstanceId = guestInstanceId
  guest.attachParams = params
  embedderElementsMap[key] = guestInstanceId
  reverseEmbedderElementsMap[guestInstanceId] = key

# Destroy an existing guest instance.
destroyGuest = (embedder, id) ->
  webViewManager.removeGuest embedder, id

  guest = guestInstances[id]?.guest
  return unless guest

  guest.destroy()
  delete guestInstances[id]

  key = reverseEmbedderElementsMap[id]
  if key?
    delete reverseEmbedderElementsMap[id]
    delete embedderElementsMap[key]

process.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_TAB_OPEN', (event, args...) ->
  [url, frameName, disposition, options] = args
  event.sender.emit 'new-window', event, url, frameName, disposition, options
  if (event.sender.isGuest() and not event.sender.allowPopups) or event.defaultPrevented
    event.returnValue = null
  else
    event.returnValue = addGuest(event.sender, createWebContents(event.sender, options))

process.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_NEXT_INSTANCE_ID', (event) ->
  event.returnValue = getNextInstanceId event.sender

process.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_REGISTER_GUEST', (event, webContents, id) ->
  guestInstances[id] = guest: webContents

ipcMain.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_ADD_GUEST', (event, id, requestId) ->
  event.sender.send "ATOM_SHELL_RESPONSE_#{requestId}", addGuest(event.sender, guestInstances[id].guest, id)

ipcMain.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_CREATE_GUEST', (event, params, requestId) ->
  event.sender.send "ATOM_SHELL_RESPONSE_#{requestId}", addGuest(event.sender, createWebContents(event.sender, params))

ipcMain.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_ATTACH_GUEST', (event, elementInstanceId, id, params) ->
  attachGuest event.sender, elementInstanceId, id, params

ipcMain.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_DESTROY_GUEST', (event, id) ->
  destroyGuest event.sender, id

ipcMain.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_SET_SIZE', (event, id, params) ->
  guestInstances[id]?.guest.setSize params

ipcMain.on 'ATOM_SHELL_GUEST_VIEW_MANAGER_SET_ALLOW_TRANSPARENCY', (event, id, allowtransparency) ->
  guestInstances[id]?.guest.setAllowTransparency allowtransparency

# Returns WebContents from its guest id.
exports.getGuest = (id) ->
  guestInstances[id]?.guest

# Returns the embedder of the guest.
exports.getEmbedder = (id) ->
  guestInstances[id]?.embedder
