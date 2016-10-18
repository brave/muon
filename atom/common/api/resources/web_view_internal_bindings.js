const TabViewInternal = require('tabViewInternal').TabViewInternal;
const remote = require('remote')

const WebViewInternal = {
  getWebContents: function (instanceId, cb) {
    if (!instanceId) {
      return
    }

    TabViewInternal.getTabID(instanceId, (tabID) => {
      if (tabID && tabID !== -1) {
        cb(remote.getWebContents(tabID))
      } else {
        // TODO(bridiver) - lastError
        console.warn('Could not find tab for guestInstanceId ' + instanceId)
        cb()
      }
    })
  },

  navigate: function (instanceId, src) {
    const webContents = WebViewInternal.getWebContents(instanceId, (webContents) => {
      if (webContents) {
        webContents.loadURL(src)
      }
    })
  },

  setAllowScaling: function (instanceId, val) {
    // ignore
  },

  setAllowTransparency: function (instanceId, val) {
    // ignore
  },

  setName: function (instanceId, val) {
    // ignore
  }
}

exports.$set('WebViewInternal', WebViewInternal);
