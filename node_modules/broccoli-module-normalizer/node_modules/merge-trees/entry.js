'use strict'

function Entry(relativePath, basePath, mode, size, mtime) {
  this.mode = mode;

  this.relativePath = relativePath;
  this.basePath = basePath;
  this.size = size;
  this.mtime = mtime;

  this.linkDir = false;
}


Entry.prototype.isDirectory = function isDirectory() {
  /*jshint -W016 */
  return (this.mode & 61440) === 16384;
};

module.exports = Entry;
