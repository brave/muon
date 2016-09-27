# componentUpdater

> register to install a default extension, or get updates to an existing extension.

Example

```javascript
const {componentUpdater} = require('electron')

// Register 1Password
componentUpdater.registerComponent('aomjjhallfgjeglblehebfpbcfeobpgk')

// Register PDFJS
componentUpdater.registerComponent('oemmndcbldboiebfnladdacbdfmadadm')

// Check for updates now for 1Password
componentUpdater.checkNow('aomjjhallfgjeglblehebfpbcfeobpgk')
```


## Methods

The `componentUpdater` module has the following methods:

### `componentUpdater.registerComponent(extensionId)`

Registers for the extension with ID `extensionId` to be installed if it does not already exist, and to be updated periodically.

### `componentUpdater.checkNow(extensionId)`

Performs an update or install now for the the extension with ID `extensionId`.
