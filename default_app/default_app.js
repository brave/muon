console.log('1')
const {app, BrowserWindow, ipcMain} = require('electron')
const path = require('path')

// app.disableHardwareAcceleration()

let mainWindow = null

// Quit when all windows are closed.
app.on('window-all-closed', () => {
  app.quit()
})

// exports.load = (appUrl) => {
  app.on('ready', () => {


    // {
    //     "minWidth":480,
    //     "minHeight":300,
    //     "titleBarStyle":"hidden-inset",
    //     "autoHideMenuBar":true,
    //     "title":"Brave",
    //     "webPreferences":
    //       {
    //         "sharedWorker":true,
    //         "nodeIntegration":false,
    //         "partition":"default",
    //         "allowFileAccessFromFileUrls":true,
    //         "allowUniversalAccessFromFileUrls":true
    //       },
    //       "frame":true,
    //       "width":721,"height":575,"x":418,"y":512,"center":false}


    const options = {
      minWidth: 480,
      minHeight: 300,
      title: 'Brave',
      titleBarStyle: 'hidden-inset',
      frame: true,
      width: 800,
      height: 600,
      autoHideMenuBar: true,
      backgroundColor: '#FFFFFF',
      useContentSize: true,
      webPreferences: {
        sharedWorker: true,
        partition: 'default',
        allowFileAccessFromFileUrls: true,
        allowUniversalAccessFromFileUrls: true
      },
      x: 418,
      y: 512,
      center: false,
      show: false
    }

    if (process.platform === 'linux') {
      options.icon = path.join(__dirname, 'icon.png')
    }

    mainWindow = new BrowserWindow(options)

    mainWindow.webContents.on('load-start', (e) => {
      console.log('load-start')
    })
    mainWindow.webContents.on('did-get-redirect-request', (e) => {
      console.log('did-get-redirect-request')
    })
    mainWindow.webContents.on('close', (e) => {
      console.log('close')
    })
    mainWindow.webContents.on('unresponive', (e) => {
      console.log('unresponive')
    })
    mainWindow.webContents.on('responsive', (e) => {
      console.log('responsive')
    })
    mainWindow.webContents.on('did-navigate', (e) => {
      console.log('did-navigate')
      // mainWindow.webContents.loadURL('http://test.com')
    })
    mainWindow.webContents.on('will-navigate', (e) => {
      console.log('will-navigate')
    })
    mainWindow.webContents.on('did-start-load', (e) => {
      console.log('did-start-load')
    })
    mainWindow.webContents.on('render-view-deleted', (e) => {
      console.log('render-view-deleted')
    })
    mainWindow.webContents.on('crashed', (e) => {
      console.log('crashed')
    })
    mainWindow.webContents.on('did-finish-load', (e) => {
      console.log('did-finish-load')
      mainWindow.webContents.openDevTools()
      mainWindow.webContents.send('test2', 'blah2')
    })
    mainWindow.show()
    // mainWindow.loadURL('http://test.com')
    // mainWindow.loadURL('file:///Users/bjohnson/Documents/brave/browser-laptop/app/extensions/brave/index-dev.html')
    mainWindow.loadURL('chrome://brave/Users/bjohnson/Documents/chromium/src/electron/default_app/index.html')
    // mainWindow.loadURL('chrome-devtools://devtools/bundled/inspector.html?remoteBase=https://chrome-devtools-frontend.appspot.com/serve_file/@e24af55b17c3554262658d03395bc48ca05deb4a/&can_dock=true&toolbarColor=rgba(223,223,223,1)&textColor=rgba(0,0,0,1)&experiments=true')
    // mainWindow.focus()

  })
// }
