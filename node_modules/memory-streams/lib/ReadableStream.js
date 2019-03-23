
//------------------------------------------------------------------
// Dependencies

var Stream = require('readable-stream')
  , util   = require('util');
  

//------------------------------------------------------------------
// Exports

module.exports = ReadableStream;


//------------------------------------------------------------------
// ReadableStream class

util.inherits(ReadableStream, Stream.Readable);

function ReadableStream (data) {
  Stream.Readable.call(this);
  this._data = data;
}

ReadableStream.prototype._read = function(n) {
  this.push(this._data);
  this._data = '';
};

ReadableStream.prototype.append = function(data) {
  this.push(data);
};