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
})

exports.$set('binding', binding.generate())
