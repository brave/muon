'use strict'

const bindings = process.atomBinding('app')
const {app, App} = bindings

// Only one app object permitted.
module.exports = app

const electron = require('electron')
const {deprecate, Menu} = electron
const {EventEmitter} = require('events')

Object.setPrototypeOf(App.prototype, EventEmitter.prototype)

let appPath = null

Object.assign(app, {
  getAppPath () { return appPath },
  setAppPath (path) { appPath = path },
  setApplicationMenu (menu) {
    return Menu.setApplicationMenu(menu)
  },
  getApplicationMenu () {
    return Menu.getApplicationMenu()
  },
  commandLine: {
    appendSwitch: bindings.appendSwitch,
    appendArgument: bindings.appendArgument
  }
})

if (process.platform === 'darwin') {
  app.dock = {
    bounce (type = 'informational') {
      return bindings.dockBounce(type)
    },
    cancelBounce: bindings.dockCancelBounce,
    downloadFinished: bindings.dockDownloadFinished,
    setBadge: bindings.dockSetBadgeText,
    getBadge: bindings.dockGetBadgeText,
    hide: bindings.dockHide,
    show: bindings.dockShow,
    isVisible: bindings.dockIsVisible,
    setMenu: bindings.dockSetMenu,
    setIcon: bindings.dockSetIcon
  }
}

if (process.platform === 'linux') {
  app.launcher = {
    setBadgeCount: bindings.unityLauncherSetBadgeCount,
    getBadgeCount: bindings.unityLauncherGetBadgeCount,
    isCounterBadgeAvailable: bindings.unityLauncherAvailable,
    isUnityRunning: bindings.unityLauncherAvailable
  }
}

app.postMessage = function (message) {
  app.emit('app-post-message', {}, message)
}

function Worker (module_name) {
  this.module_name = module_name
  this.lastError = null
  this.__onerror = null
  this.onmessage = null
}

Worker.prototype.start = function () {
  this.id = app._startWorker(this.module_name)
}

Worker.prototype.postMessage = function (message) {
  app._postMessage(this.id, message)
}

Worker.prototype.terminate = function () {
  app.stopWorker(this.id)
}

Object.defineProperty(Worker.prototype, 'onerror', {
  get: function () { return this.__onerror },
  set: function (cb) {
    this.__onerror = cb
    this.lastError && this.__onerror(this.lastError)
  }
})

Object.setPrototypeOf(Worker.prototype, EventEmitter.prototype)

app.createWorker = function (module_name) {
  const worker = new Worker(module_name)

  // It is always safe to call the worker methods because
  // WorkerThreadRegistry will return a dummy task runner
  app.on('worker-start', (e, worker_id) => {
    if (worker.id === worker_id) {
      worker.emit('start', {})
    }
  })
  app.on('worker-stop', (e, worker_id) => {
    if (worker.id === worker_id) {
      worker.emit('stop', {})
    }
  })
  app.on('worker-post-message', (e, worker_id, message) => {
    if (worker.id === worker_id) {
      const event = {data: message}
      worker.emit('message', event)
      worker.onmessage && worker.onmessage(event)
    }
  })
  app.on('worker-onerror', (e, worker_id, message, stack) => {
    worker.lastError = message
    if (worker.id === worker_id) {
      worker.onerror && worker.onerror(message, stack)
    }
  })
  app.on('app-post-message', (e, message) => {
    worker.postMessage(message)
  })

  return worker
}

app.allowNTLMCredentialsForAllDomains = function (allow) {
  if (!process.noDeprecations) {
    deprecate.warn('app.allowNTLMCredentialsForAllDomains', 'session.allowNTLMCredentialsForDomains')
  }
  let domains = allow ? '*' : ''
  if (!this.isReady()) {
    this.commandLine.appendSwitch('auth-server-whitelist', domains)
  } else {
    electron.session.defaultSession.allowNTLMCredentialsForDomains(domains)
  }
}

// Routes the events to webContents.
const events = ['login', 'certificate-error', 'select-client-certificate']
for (let name of events) {
  app.on(name, (event, webContents, ...args) => {
    webContents.emit.apply(webContents, [name, event].concat(args))
  })
}

// Wrappers for native classes.
const {DownloadItem} = process.atomBinding('download_item')
Object.setPrototypeOf(DownloadItem.prototype, EventEmitter.prototype)
