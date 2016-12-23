var binding = require('binding').Binding.create('webFrame')

var webFrameNatives = requireNative('webFrame')

binding.registerCustomHook(function(bindingsAPI, extensionId) {
  var apiFunctions = bindingsAPI.apiFunctions;
  var webFrame = bindingsAPI.compiledApi;

  apiFunctions.setHandleRequest('setGlobal', function(path, value) {
    return webFrameNatives.setGlobal(path.split('.'), value)
  })

  apiFunctions.setHandleRequest('executeJavaScript', function(code) {
    return webFrameNatives.executeJavaScript(code)
  })

  apiFunctions.setHandleRequest('setSpellCheckProvider', function(lang, autoCorrectEnabled, spellCheckProvider) {
    return webFrameNatives.setSpellCheckProvider(lang, autoCorrectEnabled, spellCheckProvider)
  })

  apiFunctions.setHandleRequest('setZoomLevel', function(level) {
    return webFrameNatives.setZoomLevel(level)
  })

  apiFunctions.setHandleRequest('setZoomLevelLimits', function(minimumLevel, maximumLevel) {
    return webFrameNatives.setZoomLevelLimits(minimumLevel, maximumLevel)
  })

  apiFunctions.setHandleRequest('setPageScaleLimits', function(minScale, maxScale) {
    return webFrameNatives.setPageScaleLimits(minScale, maxScale)
  })
})

exports.$set('binding', binding.generate())
