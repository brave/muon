'use strict'

function isPromise (val) {
  return (
    val &&
    val.then &&
    val.then instanceof Function &&
    val.constructor &&
    val.constructor.reject &&
    val.constructor.reject instanceof Function &&
    val.constructor.resolve &&
    val.constructor.resolve instanceof Function
  )
}

if (typeof module !== 'undefined') {
  module.exports = isPromise
} else {
  exports.$set('isPromise', isPromise)
}
