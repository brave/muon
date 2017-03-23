const path = requireNative('path')

const commonjs = function (fn, exports, modulePath, __global__) {
  // convert module.exports to exports.$set
  const exportsHandler = {
    set: (target, name, value) => {
      target[name] = value
      exports.$set(name, value)
      return true
    }
  }
  const setExportsHandler = {
    set: (target, name, value) => {
      // handle module.exports = mymodule
      if (name === 'exports') {
        target['__exports'] = value
        exports.$set('binding', value)
      } else {
        target[name] = value
      }
      return true
    },
    get: (target, name) => {
      if (name === 'exports' && target['__exports']) {
        return target['__exports']
      } else {
        return target[name]
      }
    }
  }
  const module = {
    exports: new Proxy({}, exportsHandler)
  }
  const moduleProxy = new Proxy(module, setExportsHandler)

  class ModuleNotFoundError extends Error {
    constructor(moduleName) {
      const message = 'Cannot find module \'' + moduleName + '\''
      super(message)
      this.name = this.constructor.name
      if (typeof Error.captureStackTrace === 'function') {
        Error.captureStackTrace(this, this.constructor)
      } else {
        this.stack = (new Error(message)).stack
      }
    }
  }

  let moduleDir = path.dirname(modulePath)
  if (moduleDir === '.') {
    moduleDir = modulePath
  }

  define(['console'], (console) => {
    const requireProxy = (moduleName) => {
      // handle relative require paths
      moduleName = moduleName.startsWith('.') && modulePath ?
        path.append(moduleDir, moduleName) : moduleName

      if (!path.require(moduleName))
        throw new ModuleNotFoundError(moduleName)

      // handle module.exports = mymodule
      const module = require(moduleName)
      return module.binding || module
    }

    try {
      fn.apply(__global__, [requireProxy, moduleProxy, console])
    } catch (e) {
      if (__global__.onerror) {
        __global__.onerror(e)
      }
      throw e
    }
  })
}

exports.$set('require', commonjs)
