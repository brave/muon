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
		"electron": "brave/electron-prebuild"
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

As part of the deprecation of asar archives, it is unavailable as a browser protocol.

Some alternatives for packaging app resources include:
1. Package resources into a Chrome Extension.
1. Include resources as flat files outside of an archive. Using `chrome://brave/*` can work as an alternate `file:///` in a renderer.

Brave uses the first solution in its [`'brave'` extension](https://github.com/brave/browser-laptop/tree/master/app/extensions/brave).

## `<webview>` element

- Not available in renderers without `chrome://brave/*` prefix.
- Attributes which are no longer supported: `allowtransparency`, `httpreferer`
- `getURL`, `send`, and other remote methods can only be called from the main process. Example in [#396](https://github.com/brave/muon/issues/396#issuecomment-358521847)

## [`protocol`](../api/protocol.md) API

- `protocol.registerFileProtocol` is removed
- `protocol.registerStreamProtocol` is not yet supported (brave/browser-laptop#10629)

## Links

- [browser-laptop](https://github.com/brave/browser-laptop) - Brave Browser built using Muon
- [muon-quick](https://github.com/brave/muon-quick) - Sample code to get started with Muon; analagous to [electron-quick-start](https://github.com/electron/electron-quick-start).

## References

<a name="ref1">[1]</a> "I think we mainly split on a philosophical difference. Electron pulled in a minimal amount of chromium code and built custom apis on top of it. We have been moving in the other direction pulling in more chromium code and exposing js apis for it" - [@bridiver](https://github.com/bridiver)
