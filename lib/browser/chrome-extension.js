const electron = require('electron')
const ipcMain = electron.ipcMain
const app = electron.app
const fs = require('fs')
const path = require('path')
const url = require('url')
const extensions = process.atomBinding('extension')
const webContents = require('electron').webContents
const BrowserWindow = require('electron').BrowserWindow;

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

// if defined(ENABLE_EXTENSIONS)
// TODO(bridiver) - turn these into modules

// List of currently active background pages by extensionId
var backgroundPages = {}

// List of events registered for background pages by extensionId
var backgroundPageEvents = {}

// List of current active tabs
var tabs = {}

// details for browser action setPopup
var browserActionPopups = {}

process.on('load-extension', function(name, base_dir, manifest_location, flags) {
  flags = flags || 0
  manifest_location = manifest_location || 'unpacked'

  var error_message = extensions.load(path.join(base_dir, name), manifest_location, flags)
  if (error_message && error_message !== '') {
    console.error(error_message)
    process.emit('did-extension-load-error', name, error_message)
  } else {
    process.emit('did-extension-load', name)
  }
})

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
    return -1;
  }
}

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
var createTab = function (evt, createProperties) {
  var windowId = createProperties.windowId;
  var win = null;
  if (windowId)
    win = BrowserWindow.fromId(windowId)
  else
    win = BrowserWindow.getFocusedWindow()

  if (!win)
    return

  var winTabs = tabsQuery({windowId: win.id})
  // createTab is fired from the background page so
  // find a guest that can be used as a "sender"
  var tab = winTabs.find((tab) => tab.active) ||
            winTabs.find((tab) => tab.isGuest())
  if (!tab)
    return

  var payload = {}
  process.emit('ELECTRON_GUEST_VIEW_MANAGER_NEXT_INSTANCE_ID', payload)
  var guestInstanceId = payload.returnValue

  var guest = webContents.create({
    isGuest: true, // new tabs are always guests, otherwise they would be new windows
    guestInstanceId,
    partition: "", // TODO(bridiver) - get partition from evt.sender using session
    embedder: win.webContents
  })

  // make sure the url is correct because it may not be loaded when this method returns
  var tabInfo = extensions.tabValue(guest)
  tabInfo.url = createProperties.url
  tabInfo.status = 'loading'

  process.emit('ELECTRON_GUEST_VIEW_MANAGER_REGISTER_GUEST', {}, guest, guestInstanceId)
  process.emit('ELECTRON_GUEST_VIEW_MANAGER_TAB_OPEN',
    { sender: tabs[tab.id].webContents }, // event
    createProperties.url,
    "",
    createProperties.active || createProperties.selected ? 'foreground-tab' : 'background-tab',
    { webPreferences: guest.getWebPreferences() })

  return tabInfo
}

var updateTab = function (tabId, updateProperties) {
  var webContents = getWebContentsForTab(tabId)
  if (!webContents)
    return

  if (updateProperties.url)
    webContents.loadURL(updateProperties.url)
  // TODO(bridiver) - implement other properties
};

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
    return;
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
  // TODO(bridiver) - add update for window id changing
  webContents.on('set-active', function (evt, active) {
    var win = webContents.getOwnerBrowserWindow()
    win && sendToBackgroundPages('all', 'chrome-tabs-activated', tabId, {windowId: win.id})
  })
})

ipcMain.on('chrome-tabs-create', function (evt, responseId, createProperties) {
  evt.sender.send('chrome-tabs-create-' + responseId, createTab(evt, createProperties))
})

ipcMain.on('chrome-tabs-query', function (evt, responseId, queryInfo) {
  var response = tabsQuery(queryInfo)
  evt.sender.send('chrome-tabs-query-response-' + responseId, response)
})

ipcMain.on('chrome-tabs-update', function (evt, responseId, tabId, updateProperties) {
  var response = updateTab(tabId, updateProperties);
  evt.sender.send('chrome-tabs-update-response-' + responseId, response);
});

['updated', 'created', 'removed', 'activated'].forEach((event_name) => {
  ipcMain.on('register-chrome-tabs-' + event_name, function (evt, extensionId) {
    addBackgroundPageEvent(extensionId, 'chrome-tabs-' + event_name)
  })
})

// chrome.windows
ipcMain.on('chrome-windows-get-current', function(evt, responseId, getInfo) {
  var response = {
    focused: false,
    incognito: false // TODO(bridiver) add incognito info to session api
  }
  if (getInfo) {
    if (getInfo.populate) {
      console.error('getWindow with tabs not supported yet')
    } else if(getInfo.windowTypes) {
      console.error('getWindow with windowTypes not supported yet')
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

ipcMain.on('chrome-windows-create', function(evt, responseId, createData) { // eslint-disable-line
  // TODO(bridiver) - implement this
  evt.sender.send('chrome-windows-create-response-' + responseId, {})
})

// chrome.browserAction
ipcMain.on('register-chrome-browser-action', function(evt, extensionId, actionTitle) {
  addBackgroundPageEvent(extensionId, 'chrome-browser-action-clicked')
  process.emit('chrome-browser-action-registered', extensionId, actionTitle)
});

ipcMain.on('chrome-browser-action-clicked', function (evt, extensionId) {  // eslint-disable-line
  var details = browserActionPopups[extensionId]
  if (details) {
    process.emit('chrome-browser-action-popup', extensionId, details)
  } else {
    var response = tabsQuery({currentWindow: true, active: true})[0]
    if (response) {
      sendToBackgroundPages(extensionId, 'chrome-browser-action-clicked', response)
    }
  }
})

ipcMain.on('chrome-browser-action-set-popup', function (evt, extensionId, details) {
  browserActionPopups[extensionId] = details
})

ipcMain.on('chrome-context-menu-create', function (evt, extensionId, properties) { // eslint-disable-line
  // TODO(bridiver) - implement this
})
// endif

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
