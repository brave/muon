const {EventEmitter} = require('events')
const {importer, Importer} = process.atomBinding('importer')

Object.setPrototypeOf(Importer.prototype, EventEmitter.prototype)

module.exports = importer
