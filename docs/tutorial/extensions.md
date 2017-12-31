Muon exposes the ability to use extensions within your electron app. While a lot of this is still highly undocumented, below is a quick guideline on how to load and manage extensions within your app.

The extension utilities are found under `session.defaultSession.extensions` and have the following API calls.

### `extensions.load(path, manifest, manifestLocation)`

This will load an extension for use. The path is absolute, and manifest is defaulted to `{}` until it locates your manifest file in your extension folder to load into the manifest object. 

You can replace `{}` with a custom manifest object if you wish to pass it, and your extension doesn't have a manifest file in it's folder. This allows you to dynamically generate your manifest.

### `extensions.enable(extensionID)`

This will enable an extension that has already been loaded via the `load()` api.

### `extensions.disable(extensionID)`

This will disable an extension that has already been loaded via the `load()` api.

### `extensionID`

The `exitensionID` is generated automatically by muon via 2 ways. 

1. If a key is presented, a hash of that key is created. This will result in always having the same `extensionID`.
2. If no key is presented, a hash of the file path to the extension will result in an `extensionID`. The extensionID will not always be the same. So it is recommended to generate and use a key. Guides can be found on that by searching `chrome extension key generation`. 

> Note: If you want to find out the id created by your key, you can manually load your extension folder into chrome under `chrome://extensions` and it will show your extensionID there. You must be in developer mode for that to work.

---

## Using an extension

Below is a lifecycle that [shadowcodex](https://github.com/shadowcodex) created.

1. Create a file called `extensions.js` and import it into your `main.js` file. 
2. Create a folder called `extensions` and manually copy your extensions into their respective folders in there.
3. Call `extensions.init()` to initialize and load the extensions for your app.

In the case of the following extension.js file, shadowcodex is using the `chrome.ipcRenderer` to call the functions via a lifecycle in the renderer thread. For example a simple vuejs app is shown:

> Note: In order to use `chrome.ipcRenderer` your render file must be loaded with `chrome://brave/` and this one in particular is loaded like `mainWindow.loadURL('chrome://brave/' + path.join(__dirname, '../client/index.html'))`

### Vuejs renderer lifecycle hook

```html
<html>
  <body>
    <div id="app">
      {{message}}
    </div>
    
    <script src="https://cdn.jsdelivr.net/npm/vue"></script>
    <script>
      var app = new Vue({
        el: '#app',
        data: {
          message: 'Hello Vue!'
        },
        mounted: function() {
          //lifecycle hook
          chrome.ipcRenderer.send('halo', '')
        }
      })
    </script>
  </body>
</html>
```

### `extension.js`

```javascript
const path = require('path')
const {ipcMain, session} = require('electron')

module.exports.init = () => {
    let hapiRegistered = false;

    ipcMain.on('halo', (event, arg) => {
        console.log(event)
        if(!hapiRegistered){
            session.defaultSession.extensions.load(path.join(__dirname,`extensions/hapi`), {}, 'unpacked');            
            hapiRegistered = true;
        } else {
            session.defaultSession.extensions.enable('mnojpmjdmbbfmejpflffifhffcmidifd')                  
        }       
    })

    ipcMain.on('unload-halo', (event, arg) => {
        console.log('unload hapi');
        session.defaultSession.extensions.disable('mnojpmjdmbbfmejpflffifhffcmidifd')                      
    })
}
```

