var ipc = require('ipc_utils')
var process = requireNative('process')
var extensionId = process.GetExtensionId()
var runtime = require('runtime').binding

var createMenuItemId = () => {
  return 'context-menus-' + extensionId + ipc.guid()
}

var createResponseId = () => {
  return extensionId + ipc.guid()
}

var binding = {
  onClicked: {
    addListener: function (cb) {
      ipc.on('chrome-context-menus-clicked', function(evt, info, tab) {
        cb(info, tab)
      })
    },
    removeListener: function (cb) {
      // TODO
      // ipc.off('chrome-context-menus-clicked', cb)
    }
  },
  remove: function (menuItemId, cb) {
    switch(typeof menuItemId) {
        case 'number':
        case 'string':
            break; // accepted cases
        default:
            throw new Error('Please define the desired context menu to remove by its number or string identifier.')
    }

    var responseId = createResponseId()
    ipc.once('chrome-context-menus-remove-response-' + responseId, function(evt) {
      if (cb) {
        cb()
      }
    })
    ipc.send('chrome-context-menus-remove', responseId, extensionId, menuItemId)
  },
  removeAll: function (cb) {
    var responseId = createResponseId()
    ipc.once('chrome-context-menus-remove-all-response-' + responseId, function(evt) {
      if (cb) {
        cb()
      }
    })
    ipc.send('chrome-context-menus-remove-all', responseId, extensionId)
  },
  create: function (properties, cb) {
    var responseId = createResponseId()
    var menuItemId = properties.id || createMenuItemId()
    if (properties.checked !== undefined) {
      throw new Error('createProperties.checked of contextMenus.create is not supported yet')
    }
    if (properties.documentUrlPatterns !== undefined) {
      throw new Error('createProperties.documentUrlPatterns of contextMenus.create is not supported yet')
    }
    if (properties.targetUrlPatterns !== undefined) {
      throw new Error('createProperties.targetUrlPatterns of contextMenus.create is not supported yet')
    }
    if (properties.enabled !== undefined) {
      throw new Error('createProperties.enabled of contextMenus.create is not supported yet')
    }
    ipc.once('chrome-context-menus-create-response-' + responseId, function(evt) {
      if (cb) {
        cb()
      }
    })
    properties.onclick && ipc.on('chrome-context-menus-clicked', (evt, info, tab) => {
      if (info.menuItemId === menuItemId) {
        properties.onclick(info, tab)
      }
    })
    let manifest = runtime.getManifest()
    let icon = manifest.icons['16']
    ipc.send('chrome-context-menus-create', responseId, extensionId, menuItemId, properties, icon)
    return menuItemId
  }
  update: function (menuItemId, properties, cb) {
    switch(typeof menuItemId) {
        case 'number':
        case 'string':
            break; // accepted cases
        default:
            throw new Error('Please define the desired context menu to remove by its number or string identifier.')
    }

    if (!properties) {
        if (cb) {
            cb();
        }
        return
    }

    var responseId = createResponseId()
    if (properties.checked !== undefined) {
      throw new Error('createProperties.checked of contextMenus.update is not supported yet')
    }
    if (properties.documentUrlPatterns !== undefined) {
      throw new Error('createProperties.documentUrlPatterns of contextMenus.update is not supported yet')
    }
    if (properties.targetUrlPatterns !== undefined) {
      throw new Error('createProperties.targetUrlPatterns of contextMenus.update is not supported yet')
    }
    if (properties.enabled !== undefined) {
      throw new Error('createProperties.enabled of contextMenus.update is not supported yet')
    }
    ipc.once('chrome-context-menus-update-response-' + responseId, function(evt) {
      if (cb) {
        cb()
      }
    })
    ipc.on('chrome-context-menus-update', (evt, info, tab) => {
      if (properties.onclick && info.menuItemId === menuItemId) {
        properties.onclick(info, tab)
      }
    })
    let manifest = runtime.getManifest()
    ipc.send('chrome-context-menus-update', responseId, extensionId, menuItemId, properties)
  }
}

exports.binding = binding

// TODO (Anthony): Move this to separated binding
var tabs = require('tabs').binding

runtime.openOptionsPage = function (cb) {
  let manifest = runtime.getManifest()
  let optionsPageURL = runtime.getURL(manifest.options_page)
  tabs.create({url: optionsPageURL})
  cb && cb()
}
