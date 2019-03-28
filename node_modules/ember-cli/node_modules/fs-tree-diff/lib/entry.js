'use strict';

var DIRECTORY_MODE = 16877;

module.exports = Entry;
function Entry(relativePath, size, mtime, mode) {
  if (arguments.length < 4) {
    var isDirectory = relativePath.charAt(relativePath.length - 1) === '/';
    this.mode = isDirectory ? DIRECTORY_MODE : 0;
  } else {
    var modeType = typeof mode;
    if (modeType !== 'number') {
      throw new TypeError('Expected `mode` to be of type `number` but was of type `' + modeType + '` instead.');
    }
    this.mode = mode;
  }

   // ----------------------------------------------------------------------
   // required properties

  this.relativePath = relativePath;
  this.size = size;
  this.mtime = mtime;
}

Entry.isDirectory = function (entry) {
  return (entry.mode & 61440) === 16384;
};

Entry.isFile = function (entry) {
  return !Entry.isDirectory(entry);
};

Entry.fromStat = function(relativePath, stat) {
  var entry = new Entry(relativePath, stat.size, stat.mtime, stat.mode);
  return entry;
};

// required methods

Entry.prototype.isDirectory = function() {
  return Entry.isDirectory(this);
};
