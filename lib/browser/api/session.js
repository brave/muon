const {EventEmitter} = require('events')
const {app} = require('electron')
const {fromPartition, Session} = process.atomBinding('session')

// Public API.
Object.defineProperties(exports, {
  defaultSession: {
    enumerable: true,
    get () { return fromPartition('') }
  },
  fromPartition: {
    enumerable: true,
    value: fromPartition
  }
})

Object.setPrototypeOf(Session.prototype, EventEmitter.prototype)

Session.prototype._init = function () {
  app.emit('session-created', this)
  console.log('updateClient-------------', this.updateClient, Object.getPrototypeOf(this.updateClient))
  Object.setPrototypeOf(Object.getPrototypeOf(this.updateClient), EventEmitter.prototype)
}
