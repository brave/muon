# Moving from Electron to Muon

Muon is a fork of Electron with improved security and support for Chrome extensions.

While both provide similar features, it's important to note that Muon has mostly diverged from Electron changes since around Electron v1.4.13 (Dec 20, 2016) due to architectural differences [[1]](#ref1).

This document details API differences which affect switching to Muon.

As Muon is actively maintained for use in [Brave Browser](https://brave.com/), it can also be useful to study the code found in [brave/browser-laptop](https://github.com/brave/browser-laptop).

## Prebuilt binaries

Prebuilt binaries for Muon are provided by the [brave/electron-prebuilt](https://github.com/brave/electron-prebuilt) repo. It can be used in `package.json` along with related variables configured in `.npmrc`. Running `npm install && npm rebuild` after adding the following changes should switch your project to use Muon.

`package.json`
```json
{
	"devDependencies": {
		"electron": "brave/electron-prebuilt"
	}
}
```

`.npmrc`
```
runtime = electron
target_arch = x64
brave_electron_version = 4.7.2
chromedriver_version = 2.33
target = v4.7.2
disturl=https://brave-laptop-binaries.s3.amazonaws.com/atom-shell/dist/
build_from_source = true
```

See [`.npmrc` in brave/browser-laptop](https://github.com/brave/browser-laptop/blob/master/.npmrc) for up-to-date versions.

### Building from source

Refer to the [browser-laptop-bootstrap](https://github.com/brave/browser-laptop-bootstrap) repo for instructions on building Muon from source.

## Process sandbox

Muon removes NodeJS integration from the renderer process to improve security by enabling [Chromium's Sandbox features](https://chromium.googlesource.com/chromium/src/+/b4730a0c2773d8f6728946013eb812c6d3975bec/docs/design/sandbox.md).

A limited subset of Electron APIs are still available when files are loaded using the `chrome://brave/*` protocol. The wildcard path accepts an absolute file path and works similar to the `file:///*` protocol.

APIs are available under the `chrome.*` global object. Among those included are [`ipcRenderer`](../api/ipc-renderer.md), [`remote`](../api/remote.md), and [`webFrame`](../api/web-frame.md).

### Webpack configuration

Using webpack's [`externals`](https://webpack.js.org/configuration/externals/) config can provide an easier transition instead of replacing all `'electron'` imports.
```js
externals: {
    'electron': 'chrome'
}
```

## asar archives

[Electron's asar archives](https://github.com/electron/asar) are deprecated in Muon and will be removed sometime in the future. They're planned to be replaced with pak files.

Currently Brave uses them to archive extensions which are to be unpacked on first startup.

### Removed `asar://` protocol

Accessing contents of an asar archive is no longer possible using the `asar://` protocol. In its place is the `chrome://brave/` protocol which supports loading files from within `.asar` archives.

In the renderer context, the `__dirname` global is unavailable due to removed NodeJS integration. To link to files within the asar archive, use paths relative to your app's entry html file—similar to a typical web application.

Given an application structure of
```
app/
  assets/icon.svg
  index.html
```

Using `icon.svg` can be linked using a relative path.
```html
<img src="./assets/icon.svg" />
```
The resolved url would resemble `chrome://brave//Users/Foobar/MyApp/app/assets/icon.svg`. Keep in mind, the window frame's location must be using the `chrome://brave/` protocol for this to work.

## `<webview>` element

- Not available in renderers without `chrome://brave/*` prefix.
- Attributes which are no longer supported: `preload`, `allowtransparency`, `httpreferer`
- `getURL`, `send`, and other remote methods can only be called from the main process. Example in [#396](https://github.com/brave/muon/issues/396#issuecomment-358521847)

## `preload` scripts

Preload scripts have been removed in favor of [Chrome extension content scripts](https://developer.chrome.com/extensions/content_scripts). 

The `ipcRenderer` API is still available under the `chrome.*` object in content scripts.

### Mimicking a preload script from a Chrome extension

If mutating any `window.*` global object is important for your project, it can be closely mimicked by injecting a JavaScript file from your content script.

```js
const script = document.createElement('script');
script.src = chrome.runtime.getURL('preload.js');
document.documentElement.appendChild(script);
```

Set the [extension manifest](https://developer.chrome.com/extensions/content_scripts#registration)'s `run_at` property to use the value of `'document_start'` in combination with the above script to closely match the scope and timing of Electron's preload scripts.

## [`protocol`](../api/protocol.md) API

- `protocol.registerFileProtocol` is removed
- `protocol.registerStreamProtocol` is not yet supported ([brave/browser-laptop#10629](https://github.com/brave/browser-laptop/issues/10629))

## WidevineCDM

Muon supports downloading and using WidevineCDM binaries with the [`componentUpdater`](../api/component-updater.md) API.

Add the following code to the main process for WidevineCDM support.
```js
const widevineComponentId = 'oimompecagnajdejgnnjijobebaeigek'
const widevineComponentPublicKey = 'MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCmhe+02cLPPAViaevk/fzODKUnb/ysaAeD8lpE9pwirV6GYOm+naTo7xPOCh8ujcR6Ryi1nPTq2GTG0CyqdDyOsZ1aRLuMZ5QqX3dJ9jXklS0LqGfosoIpGexfwggbiLvQOo9Q+IWTrAO620KAzYU0U6MV272TJLSmZPUEFY6IGQIDAQAB'

componentUpdater.on('component-registered', (event, componentId) => {
  componentUpdater.checkNow(componentId)
})

componentUpdater.registerComponent(widevineComponentId, widevineComponentPublicKey)
```

## Links

- [browser-laptop](https://github.com/brave/browser-laptop) - Brave Browser built using Muon
- [muon-quick](https://github.com/brave/muon-quick) - Sample code to get started with Muon; analagous to [electron-quick-start](https://github.com/electron/electron-quick-start).

## References

<a name="ref1">[1]</a> "I think we mainly split on a philosophical difference. Electron pulled in a minimal amount of chromium code and built custom apis on top of it. We have been moving in the other direction pulling in more chromium code and exposing js apis for it" - [@bridiver](https://github.com/bridiver)
