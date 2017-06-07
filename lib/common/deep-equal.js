/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const deepEqual = (x, y) => {
  if (typeof x !== typeof y) {
    return false
  }
  if (typeof x !== 'object') {
    return x === y
  }
  const xKeys = Object.keys(x)
  const yKeys = Object.keys(y)
  if (xKeys.length !== yKeys.length) {
    return false
  }
  for (let prop in x) {
    if (x.hasOwnProperty(prop)) {
      if (!deepEqual(x[prop], y[prop])) {
        return false
      }
    }
  }
  return true
}

module.exports = deepEqual
