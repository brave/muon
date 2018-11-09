const bindings = process.atomBinding('component_updater')
const {componentUpdater, ComponentUpdater} = bindings

module.exports = componentUpdater

const {EventEmitter} = require('events')

Object.setPrototypeOf(ComponentUpdater.prototype, EventEmitter.prototype)
