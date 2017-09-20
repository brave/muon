# dialog

> Display native system dialogs for opening and saving files, alerting, etc.

An example of showing a dialog to select multiple files and directories:

```javascript
const {dialog, BrowserWindow} = require('electron')
dialog.showDialog(BrowserWindow.getFocusedWindow(),
                              {type: 'select-saveas-file'},
                              (files) => {
                                console.log(files)
                              })
```

## Methods

The `dialog` module has the following methods:

### `dialog.showDialog(browserWindow, options, callback)`

* `browserWindow` BrowserWindow
* `options` Object
  * `defaultPath` String
  * `extensions` Array - Array of grouped array of extensions
  * `extensionDescriptionOverrides` Array  - Overrides the system descriptions
    of the specified extensions. Entries correspond to `extensions`;
    if left blank the system descriptions will be used.
  * `includeAllFiles` Boollean - Show all files in extensions
  * `type` String - Indicates which features the dialog should use, can
    be `select-folder`, `select-upload-folder`, `select-saveas-file`,
    `select-open-file` and `select-open-multi-file`.
* `callback` Function

On success this method returns an array of file paths chosen by the user,
otherwise it returns `undefined`.

The `extensions` specifies an array of file types that can be displayed or
selected when you want to limit the user to a specific type.
The first element of `extensions` array will be default extension.
For example:

```javascript
{
  extensions: [['jpg', 'png', 'gif'], ['html', htm'], ['txt']]
}
```

The `extensions` array should contain extensions without wildcards or dots (e.g.
`'png'` is good but `'.png'` and `'*.png'` are bad).

You can override system descriptions of extensions
```javascript
{
  extensions: [['jpg', 'png', 'gif'], ['html', htm'], ['txt'], ['brave']],
  extensionDescriptionOverrides: ['IMAGE', '', 'text file', 'Amazing Ext']
}
```

If a `callback` is passed, the API call will be asynchronous and the result
will be passed via `callback(filenames)`

For `select-saveas-file`, the filename will be the first element of the
filename array(filename[0])

If you don't specify `defaultPath`, it will be your download path (set by
`download.default_directory` user prefs).

### `dialog.showMessageBox([browserWindow, ]options[, callback])`

* `browserWindow` BrowserWindow (optional)
* `options` Object
  * `type` String - Can be `"none"`, `"info"`, `"error"`, `"question"` or
  `"warning"`. On Windows, "question" displays the same icon as "info", unless
  you set an icon using the "icon" option.
  * `buttons` Array - Array of texts for buttons. On Windows, an empty array
    will result in one button labeled "OK".
  * `defaultId` Integer - Index of the button in the buttons array which will
    be selected by default when the message box opens.
  * `title` String - Title of the message box, some platforms will not show it.
  * `message` String - Content of the message box.
  * `detail` String - Extra information of the message.
  * `icon` [NativeImage](native-image.md)
  * `cancelId` Integer - The value will be returned when user cancels the dialog
    instead of clicking the buttons of the dialog. By default it is the index
    of the buttons that have "cancel" or "no" as label, or 0 if there is no such
    buttons. On macOS and Windows the index of "Cancel" button will always be
    used as `cancelId`, not matter whether it is already specified.
  * `noLink` Boolean - On Windows Electron will try to figure out which one of
    the `buttons` are common buttons (like "Cancel" or "Yes"), and show the
    others as command links in the dialog. This can make the dialog appear in
    the style of modern Windows apps. If you don't like this behavior, you can
    set `noLink` to `true`.
* `callback` Function

Shows a message box, it will block the process until the message box is closed.
It returns the index of the clicked button.

If a `callback` is passed, the API call will be asynchronous and the result
will be passed via `callback(response)`.

### `dialog.showErrorBox(title, content)`

* `title` String - The title to display in the error box
* `content` String - The text content to display in the error box

Displays a modal dialog that shows an error message.

This API can be called safely before the `ready` event the `app` module emits,
it is usually used to report errors in early stage of startup.  If called
before the app `ready`event on Linux, the message will be emitted to stderr,
and no GUI dialog will appear.

## Sheets

On macOS, dialogs are presented as sheets attached to a window if you provide
a `BrowserWindow` reference in the `browserWindow` parameter, or modals if no
window is provided.

You can call `BrowserWindow.getCurrentWindow().setSheetOffset(offset)` to change
the offset from the window frame where sheets are attached.
