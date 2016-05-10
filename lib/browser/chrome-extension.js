const electron = require('electron')
const ipcMain = electron.ipcMain
const app = electron.app
const fs = require('fs')
const path = require('path')
const url = require('url')
const extensions = process.atomBinding('extension')
const webContents = require('electron').webContents
const BrowserWindow = require('electron').BrowserWindow

// Mapping between hostname and file path.
var hostPathMap = {}
var hostPathMapNextKey = 0

var getHostForPath = function (path) {
  var key
  key = 'extension-' + (++hostPathMapNextKey)
  hostPathMap[key] = path
  return key
}

// Cache extensionInfo.
var extensionInfoMap = {}

var getExtensionInfoFromPath = function (srcDirectory) {
  var manifest, page
  manifest = JSON.parse(fs.readFileSync(path.join(srcDirectory, 'manifest.json')))
  if (extensionInfoMap[manifest.name] == null) {
    // We can not use 'file://' directly because all resources in the extension
    // will be treated as relative to the root in Chrome.
    page = url.format({
      protocol: 'chrome-extension',
      slashes: true,
      hostname: getHostForPath(srcDirectory),
      pathname: manifest.devtools_page
    })
    extensionInfoMap[manifest.name] = {
      startPage: page,
      name: manifest.name,
      srcDirectory: srcDirectory,
      exposeExperimentalAPIs: true
    }
    return extensionInfoMap[manifest.name]
  }
}

var getResourceURL = function (extensionId, path) {
  path = String(path)
  if (!path.length || path[0] != '/')
    path = '/' + path
  return 'chrome-extension://' + extensionId + path
}

// The loaded extensions cache and its persistent path.
var loadedExtensions = null
var loadedExtensionsPath = null

app.on('will-quit', function () {
  try {
    loadedExtensions = Object.keys(extensionInfoMap).map(function (key) {
      return extensionInfoMap[key].srcDirectory
    })
    if (loadedExtensions.length > 0) {
      try {
        fs.mkdirSync(path.dirname(loadedExtensionsPath))
      } catch (error) {
        // Ignore error
      }
      fs.writeFileSync(loadedExtensionsPath, JSON.stringify(loadedExtensions))
    } else {
      fs.unlinkSync(loadedExtensionsPath)
    }
  } catch (error) {
    // Ignore error
  }
})

// List of currently active background pages by extensionId
var backgroundPages = {}

// List of events registered for background pages by extensionId
var backgroundPageEvents = {}

// List of current active tabs
var tabs = {}

process.on('enable-extension', function (extensionId) {
  extensions.enable(extensionId)
})

process.on('disable-extension', function (extensionId) {
  extensions.disable(extensionId)
})

process.on('load-extension', function (name, base_dir, options, cb) {
  if (options instanceof Function) {
    cb = options
    options = {}
  }

  manifest = options.manifest || {}
  manifest_location = options.manifest_location || 'unpacked'
  flags = options.flags || 0

  var install_info = extensions.load(path.join(base_dir, name), manifest, manifest_location, flags)
  cb && cb(install_info)
})

// TODO(bridiver) move these into modules
// background pages
var addBackgroundPage = function (extensionId, backgroundPage) {
  backgroundPages[extensionId] = [backgroundPage]
}

var addBackgroundPageEvent = function (extensionId, event) {
  backgroundPageEvents[event] = backgroundPageEvents[event] || []
  if (backgroundPageEvents[event].indexOf(extensionId) === -1) {
    backgroundPageEvents[event].push(extensionId)
  }
}

var sendToBackgroundPages = function (extensionId, event) {
  if (!backgroundPageEvents[event])
    return

  var pages = []
  if (extensionId === 'all') {
    pages = backgroundPageEvents[event].reduce(
      (curr, id) => curr = curr.concat(backgroundPages[id] || []), [])
  } else {
    pages = backgroundPages[extensionId] || []
  }
  var args = [].slice.call(arguments, 1)
  pages.forEach(function (backgroundPage) {
    try {
      backgroundPage.send.apply(backgroundPage, args)
    } catch (e) {
      console.error('Could not send to background page: ' + e)
    }
  })
}

var createBackgroundPage = function (webContents) {
  var extensionId = webContents.getURL().match(/chrome-extension:\/\/([^\/]*)/)[1]
  addBackgroundPage(extensionId, webContents)
  webContents.on('destroyed', function () {
    var index = backgroundPages[extensionId].indexOf(webContents)
    if (index > -1) {
      backgroundPages[extensionId].splice(index, 1)
    }
  })
}

// chrome.tabs
var getWebContentsForTab = function (tabId) {
  return tabs[tabId].webContents
}

var getTabValue = function (tabId) {
  var webContents = getWebContentsForTab(tabId)
  if (webContents) {
    return extensions.tabValue(webContents)
  }
}

var getWindowIdForTab = function (tabId) {
  // cache the windowId so we can still access
  // it when the webContents is destroyed
  if (tabs[tabId] && tabs[tabId].windowId) {
    return tabs[tabId].windowId
  } else {
    return -1
  }
}

var getTabsForWindow = function (windowId) {
  return tabsQuery({windowId})
}

var createGuest = function (opener) {
  var payload = {}
  process.emit('ELECTRON_GUEST_VIEW_MANAGER_NEXT_INSTANCE_ID', payload)
  var guestInstanceId = payload.returnValue

  var options = {
    guestInstanceId,
    session: opener.session,
    isGuest: true,
    embedder: opener
  }
  var webPreferences = Object.assign({}, opener.getWebPreferences(), options)
  var guest = webContents.create(webPreferences)

  process.emit('ELECTRON_GUEST_VIEW_MANAGER_REGISTER_GUEST', { sender: opener }, guest, guestInstanceId)
  return guest
}

var createTab = function (createProperties) {
  var opener = null
  var openerTabId = createProperties.openerTabId
  if (openerTabId) {
    opener = getWebContentsForTab(openerTabId)
  }

  var windowId = createProperties.windowId
  if (opener) {
    var win = null
    win = BrowserWindow.fromWebContents(opener)
    if (win && win.id !== windowId) {
      console.warn('The openerTabId is not in the selected window ', createProperties)
      return
    }
    if (!win) {
      console.warn('The openerTabId is not attached to a window ', createProperties)
      return
    }
  } else {
    var tab = tabsQuery({windowId: windowId || -2, active: true})[0]
    if (tab)
      opener = getWebContentsForTab(tab.id)
  }

  if (!opener) {
    console.warn('Could not find an opener for new tab ', createProperties)
    return
  }

  if (opener.isGuest()) {
    var guest = createGuest(opener)

    if (createProperties.url) {
      // make sure the url is correct because it may not be loaded when this method returns
      var tabInfo = extensions.tabValue(guest)
      tabInfo.url = createProperties.url
      tabInfo.status = 'loading'
    }

    process.emit('ELECTRON_GUEST_VIEW_MANAGER_TAB_OPEN',
      { sender: opener }, // event
      createProperties.url || 'about:blank',
      '',
      createProperties.active || createProperties.selected ? 'foreground-tab' : 'background-tab',
      { webPreferences: guest.getWebPreferences() })

    return tabInfo
  } else {
    // TODO(bridiver) - open a new window
    // ELECTRON_GUEST_WINDOW_MANAGER_WINDOW_OPEN
    console.warn('chrome.tabs.create from a non-guest opener is not supported yet')
  }
}

var removeTabs = function (tabIds) {
  Array(tabIds).forEach((tabId) => {
    var webContents = getWebContentsForTab(tabId)
    if (webContents) {
      if (webContents.isGuest()) {
        webContents.destroy()
      } else {
        var win = BrowserWindow.fromWebContents(webContents)
        win && win.close()
      }
    }
  })
}

var updateTab = function (tabId, updateProperties) {
  var webContents = getWebContentsForTab(tabId)
  if (!webContents)
    return

  if (updateProperties.url)
    webContents.loadURL(updateProperties.url)
  if (updateProperties.active || updateProperties.selected || updateProperties.highlighted)
    webContents.setActive(true)
}

var chromeTabsUpdated = function (tabId, changeInfo) {
  var tabValue = getTabValue(tabId)
  if (tabValue) {
    sendToBackgroundPages('all', 'chrome-tabs-updated', tabId, changeInfo, tabValue)
  }
}

var chromeTabsRemoved = function (tabId) {
  var windowId = getWindowIdForTab(tabId)
  delete tabs[tabId]
  sendToBackgroundPages('all', 'chrome-tabs-removed', tabId, {
    windowId: windowId,
    isWindowClosing: windowId === -1 ? true : false
  })
}

var tabsQuery = function (queryInfo, useCurrentWindowId = false) {
  var tabIds = Object.keys(tabs)

  // convert current window identifier to the actual current window id
  if (queryInfo.windowId === -2 || queryInfo.currentWindow === true) {
    delete queryInfo.currentWindow
    var focusedWindow = electron.BrowserWindow.getFocusedWindow()
    if (focusedWindow) {
      queryInfo.windowId = focusedWindow.id
    }
  }

  var queryKeys = Object.keys(queryInfo)
  // get the values for all tabs
  var tabValues = tabIds.reduce((tabs, tabId) => {
    tabs[tabId] = getTabValue(tabId)
    return tabs
  }, {})
  var result = []
  tabIds.forEach((tabId) => {
    // delete tab from the list if any key doesn't match
    if (!queryKeys.map((queryKey) => (tabValues[tabId][queryKey] === queryInfo[queryKey])).includes(false)) {
      result.push(tabValues[tabId])
    }
  })

  return result
}

process.on('web-contents-created', function (webContents) {
  if (extensions.isBackgroundPage(webContents)) {
    createBackgroundPage(webContents)
    return
  }

  var tabId = webContents.getId()
  if (tabId === -1)
    return

  tabs[tabId] = {}
  tabs[tabId].webContents = webContents

  webContents.on('did-attach', function () {
    var win = webContents.getOwnerBrowserWindow()
    tabs[tabId].windowId = win ? win.id : -2
  })

  sendToBackgroundPages('all', 'chrome-tabs-created', extensions.tabValue(webContents))

  webContents.on('destroyed', function () {
    chromeTabsRemoved(tabId)
  })
  webContents.on('dom-ready', function (evt, url) {
    chromeTabsUpdated(tabId, {status: 'complete'})
  })
  webContents.on('did-finish-load', function (evt, url) {
    chromeTabsUpdated(tabId, {url: url})
  })
  webContents.on('set-active', function (evt, active) {
    var win = webContents.getOwnerBrowserWindow()
    if (win && active)
      sendToBackgroundPages('all', 'chrome-tabs-activated', tabId, {windowId: win.id})
  })
})

ipcMain.on('chrome-tabs-create', function (evt, responseId, createProperties) {
  var response = createTab(createProperties)
  evt.sender.send('chrome-tabs-create-' + responseId, response)
})

ipcMain.on('chrome-tabs-remove', function (evt, responseId, tabIds) {
  removeTabs(tabIds)
  evt.sender.send('chrome-tabs-remove-' + responseId)
})

ipcMain.on('chrome-tabs-query', function (evt, responseId, queryInfo) {
  var response = tabsQuery(queryInfo)
  evt.sender.send('chrome-tabs-query-response-' + responseId, response)
})

ipcMain.on('chrome-tabs-update', function (evt, responseId, tabId, updateProperties) {
  var response = updateTab(tabId, updateProperties)
  evt.sender.send('chrome-tabs-update-response-' + responseId, response)
})

var tabEvents = ['updated', 'created', 'removed', 'activated']
tabEvents.forEach((event_name) => {
  ipcMain.on('register-chrome-tabs-' + event_name, function (evt, extensionId) {
    addBackgroundPageEvent(extensionId, 'chrome-tabs-' + event_name)
  })
})

// chrome.windows
ipcMain.on('chrome-windows-get-current', function (evt, responseId, getInfo) {
  var response = {
    focused: false,
    incognito: false // TODO(bridiver)
  }
  if (getInfo) {
    if (getInfo.populate) {
      console.warn('getWindow with tabs not supported yet')
    } else if(getInfo.windowTypes) {
      console.warn('getWindow with windowTypes not supported yet')
    }
  }
  var focusedWindow = electron.BrowserWindow.getFocusedWindow()
  if (focusedWindow) {
    response.id = focusedWindow.id
    response.focused = focusedWindow.isFocused()
    var bounds = focusedWindow.getBounds()
    response.top = bounds.y
    response.left = bounds.x
    response.width = bounds.width
    response.height = bounds.height
    response.alwaysOnTop = focusedWindow.isAlwaysOnTop()
  }

  evt.sender.send('chrome-windows-get-current-response-' + responseId,
    response)
})

// chrome.browserAction
var browserActionPopups = {}

ipcMain.on('register-chrome-browser-action', function (evt, extensionId, actionTitle) {
  addBackgroundPageEvent(extensionId, 'chrome-browser-action-clicked')
  process.emit('chrome-browser-action-registered', extensionId, actionTitle)
})

ipcMain.on('chrome-browser-action-clicked', function (evt, extensionId, name, props) {
  var popup = browserActionPopups[extensionId]
  if (popup) {
    process.emit('chrome-browser-action-popup', extensionId, name, props, getResourceURL(extensionId, popup))
  } else {
    var response = tabsQuery({currentWindow: true, active: true})[0]
    if (response) {
      sendToBackgroundPages(extensionId, 'chrome-browser-action-clicked', response)
    }
  }
})

ipcMain.on('chrome-browser-action-set-popup', function (evt, extensionId, details) {
  if (details.popup) {
    browserActionPopups[extensionId] = details.popup
  }
})

// We can not use protocol or BrowserWindow until app is ready.
app.once('ready', function () {
  var BrowserWindow, i, init, len, srcDirectory
  BrowserWindow = electron.BrowserWindow

  // Load persisted extensions.
  loadedExtensionsPath = path.join(app.getPath('userData'), 'DevTools Extensions')
  try {
    loadedExtensions = JSON.parse(fs.readFileSync(loadedExtensionsPath))
    if (!Array.isArray(loadedExtensions)) {
      loadedExtensions = []
    }

    // Preheat the extensionInfo cache.
    for (i = 0, len = loadedExtensions.length; i < len; i++) {
      srcDirectory = loadedExtensions[i]
      getExtensionInfoFromPath(srcDirectory)
    }
  } catch (error) {
    // Ignore error
  }

  BrowserWindow.prototype._loadDevToolsExtensions = function (extensionInfoArray) {
    var ref
    return (ref = this.devToolsWebContents) != null ? ref.executeJavaScript('DevToolsAPI.addExtensions(' + (JSON.stringify(extensionInfoArray)) + ');') : void 0
  }
  BrowserWindow.addDevToolsExtension = function (srcDirectory) {
    var extensionInfo, j, len1, ref, window
    extensionInfo = getExtensionInfoFromPath(srcDirectory)
    if (extensionInfo) {
      ref = BrowserWindow.getAllWindows()
      for (j = 0, len1 = ref.length; j < len1; j++) {
        window = ref[j]
        window._loadDevToolsExtensions([extensionInfo])
      }
      return extensionInfo.name
    }
  }
  BrowserWindow.removeDevToolsExtension = function (name) {
    return delete extensionInfoMap[name]
  }

  // Load persisted extensions when devtools is opened.
  init = BrowserWindow.prototype._init
  BrowserWindow.prototype._init = function () {
    init.call(this)
    return this.on('devtools-opened', function () {
      return this._loadDevToolsExtensions(Object.keys(extensionInfoMap).map(function (key) {
        return extensionInfoMap[key]
      }))
    })
  }
})
