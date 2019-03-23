
//------------------------------------------------------------------
// Dependencies

var Stream = require('readable-stream')
  , util   = require('util');


//------------------------------------------------------------------
// Exports

module.exports = WritableStream;
  
  
//------------------------------------------------------------------
// WritableStream class

util.inherits(WritableStream, Stream.Writable);

function WritableStream (options) {
  Stream.Writable.call(this, options);
}

WritableStream.prototype.write = function(chunk, encoding, callback) {
  var ret = Stream.Writable.prototype.write.apply(this, arguments);
  if (!ret) this.emit('drain');
  return ret;
}

WritableStream.prototype._write = function(chunk, encoding, callback) {
  this.write(chunk, encoding, callback);
};

WritableStream.prototype.toString = function() {  
  return this.toBuffer().toString();
};

WritableStream.prototype.toBuffer = function() {
  var buffers = [];
  this._writableState.buffer.forEach(function(data) {
    buffers.push(data.chunk);
  });
  
  return Buffer.concat(buffers);
};

WritableStream.prototype.end = function(chunk, encoding, callback) {
  var ret = Stream.Writable.prototype.end.apply(this, arguments);
  // In memory stream doesn't need to flush anything so emit `finish` right away
  // base implementation in Stream.Writable doesn't emit finish
  this.emit('finish');
  return ret;
};
