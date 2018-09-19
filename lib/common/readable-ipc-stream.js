const Readable = require('stream').Readable
const util = require('util')

function ReadableIpcStream (ipcMain, channel, streamOpts) {
  if (!(this instanceof ReadableIpcStream)) {
    return new ReadableIpcStream(ipcMain, channel, streamOpts)
  }

  streamOpts = streamOpts || {}
  streamOpts.objectMode = streamOpts.objectMode ? streamOpts.objectMode : true

  const onData = (event, data) => {
    data = JSON.parse(data, (_, v) => v.type === 'Buffer' ? Buffer.from(v.data) : v)
    this.push(data)
  }

  const onError = (event, data) => {
    ipcMain
      .removeListener(channel + '-data', onData)
      .removeListener(channel + '-end', onEnd)

    this.emit('error', Object.assign(new Error(`Stream error in ${channel}`), data))
  }

  const onEnd = () => {
    ipcMain
      .removeListener(channel + '-data', onData)
      .removeListener(channel + '-error', onError)

    this.push(null)
  }

  ipcMain
    .on(channel + '-data', onData)
    .once(channel + '-error', onError)
    .once(channel + '-end', onEnd)

  Readable.call(this, streamOpts)
}
util.inherits(ReadableIpcStream, Readable)

ReadableIpcStream.prototype._read = () => {}

module.exports = ReadableIpcStream
