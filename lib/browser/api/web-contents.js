'use strict'

const {EventEmitter} = require('events')
const electron = require('electron')
const {app, ipcMain, session, NavigationController, BrowserWindow} = electron
// Load the guest view manager.
const guestViewManager = require('../guest-view-manager')


// session is not used here, the purpose is to make sure session is initalized
// before the webContents module.
session

let nextId = 0
const getNextId = function () {
  return ++nextId
}

// Stock page sizes
const PDFPageSizes = {
  A5: {
    custom_display_name: 'A5',
    height_microns: 210000,
    name: 'ISO_A5',
    width_microns: 148000
  },
  A4: {
    custom_display_name: 'A4',
    height_microns: 297000,
    name: 'ISO_A4',
    is_default: 'true',
    width_microns: 210000
  },
  A3: {
    custom_display_name: 'A3',
    height_microns: 420000,
    name: 'ISO_A3',
    width_microns: 297000
  },
  Legal: {
    custom_display_name: 'Legal',
    height_microns: 355600,
    name: 'NA_LEGAL',
    width_microns: 215900
  },
  Letter: {
    custom_display_name: 'Letter',
    height_microns: 279400,
    name: 'NA_LETTER',
    width_microns: 215900
  },
  Tabloid: {
    height_microns: 431800,
    name: 'NA_LEDGER',
    width_microns: 279400,
    custom_display_name: 'Tabloid'
  }
}

// Default printing setting
const defaultPrintingSetting = {
  pageRage: [],
  mediaSize: {},
  landscape: false,
  color: 2,
  headerFooterEnabled: false,
  marginsType: 0,
  isFirstRequest: false,
  requestID: getNextId(),
  previewModifiable: true,
  printToPDF: true,
  printWithCloudPrint: false,
  printWithPrivet: false,
  printWithExtension: false,
  deviceName: 'Save as PDF',
  generateDraftData: true,
  fitToPageEnabled: false,
  duplex: 0,
  copies: 1,
  collate: true,
  shouldPrintBackgrounds: false,
  shouldPrintSelectionOnly: false,
  scaleFactor: 100,
  rasterizePDF: false
}

// JavaScript implementations of WebContents.
const binding = process.atomBinding('web_contents')
const {WebContents} = binding

Object.setPrototypeOf(NavigationController.prototype, EventEmitter.prototype)
Object.setPrototypeOf(WebContents.prototype, NavigationController.prototype)

// WebContents::send(channel, args..)
WebContents.prototype.sendShared = function (channel, shared) {
  if (channel == null) throw new Error('Missing required `channel` argument')
  if (shared == null) throw new Error('Missing required `shared` argument')
  return this._sendShared(channel, shared)
}
WebContents.prototype.send = function (channel, ...args) {
  if (channel == null) throw new Error('Missing required `channel` argument')
  return this._send(channel, args)
}

WebContents.prototype.clone = function(...args) {
  if (args.length === 0) {
    this._clone(() => {})
  } else {
    this._clone(...args)
  }
}

WebContents.prototype.detach = function(cb) {
  cb && this.once('did-detach', cb)
  return this._detachGuest()
}

WebContents.prototype.attach = function(windowId, cb) {
  cb && this.once('did-attach', cb)
  const attach = this._attachGuest(windowId)
  this.off('did-attach', cb)
  return attach
}

// Translate the options of printToPDF.
WebContents.prototype.printToPDF = function (options, callback) {
  const printingSetting = Object.assign({}, defaultPrintingSetting)
  printingSetting.previewUIID = getNextId()

  if (options.landscape) {
    printingSetting.landscape = options.landscape
  }
  if (options.marginsType) {
    printingSetting.marginsType = options.marginsType
  }
  if (options.printSelectionOnly) {
    printingSetting.shouldPrintSelectionOnly = options.printSelectionOnly
  }
  if (options.printBackground) {
    printingSetting.shouldPrintBackgrounds = options.printBackground
  }

  if (options.pageSize) {
    const pageSize = options.pageSize
    if (typeof pageSize === 'object') {
      if (!pageSize.height || !pageSize.width) {
        return callback(new Error('Must define height and width for pageSize'))
      }
      // Dimensions in Microns
      // 1 meter = 10^6 microns
      printingSetting.mediaSize = {
        name: 'CUSTOM',
        custom_display_name: 'Custom',
        height_microns: pageSize.height,
        width_microns: pageSize.width
      }
    } else if (PDFPageSizes[pageSize]) {
      printingSetting.mediaSize = PDFPageSizes[pageSize]
    } else {
      return callback(new Error(`Does not support pageSize with ${pageSize}`))
    }
  } else {
    printingSetting.mediaSize = PDFPageSizes['A4']
  }

  this._printToPDF(printingSetting, callback)
}

// Add JavaScript wrappers for WebContents class.
WebContents.prototype._init = function () {
  // The navigation controller.
  NavigationController.call(this, this)

  // Every remote callback from renderer process would add a listenter to the
  // will-destroy event, so ignore the listenters warning.
  this.setMaxListeners(0)

  // Dispatch IPC messages to the ipc module.
  this.on('ipc-message', function (event, [channel, ...args]) {
    ipcMain.emit(channel, event, ...args)
  })
  this.on('ipc-message-sync', function (event, [channel, ...args]) {
    Object.defineProperty(event, 'returnValue', {
      set: function (value) {
        return event.sendReply(JSON.stringify(value))
      },
      get: function () {}
    })
    ipcMain.emit(channel, event, ...args)
  })

  // Handle context menu action request from pepper plugin.
  this.on('pepper-context-menu', function (event, params) {
    // Access Menu via electron.Menu to prevent circular require
    const menu = electron.Menu.buildFromTemplate(params.menu)
    menu.popup(this, params.x, params.y)
  })

  // The devtools requests the webContents to reload.
  this.on('devtools-reload-page', function () {
    this.reload()
  })

  // Handle tabs getting added to windows.
  // 'tab-inserted-at' fires on both the window and the tab,
  // but the window event gives us access to both the tab and window WebContents
  if (!this.isGuest()) {
    this.on('tab-inserted-at', (event, webContents, index, active) => {
      guestViewManager.registerGuest(webContents, this)
    })
  }

  if (!this.isRemote()) {
    app.emit('web-contents-created', {}, this)
  }
}

// JavaScript wrapper of Debugger.
const {Debugger} = process.atomBinding('debugger')

Object.setPrototypeOf(Debugger.prototype, EventEmitter.prototype)

// Public APIs.
module.exports = {
  create (options = {}) {
    return binding.create(options)
  },

  fromId (id) {
    return binding.fromId(id)
  },

  createTab (...args) {
    binding.createTab(...args)
  },

  fromTabID (tabID) {
    if (!tabID)
      return
    return binding.fromTabID(tabID)
  },

  getFocusedWebContents () {
    let focused = null
    for (let contents of binding.getAllWebContents()) {
      if (!contents.isFocused()) continue
      if (focused == null) focused = contents
      // Return webview web contents which may be embedded inside another
      // web contents that is also reporting as focused
      if (contents.getType() === 'webview') return contents
    }
    return focused
  },

  getAllWebContents () {
    return binding.getAllWebContents()
  }
}
