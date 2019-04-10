const RollingFileWriteStream = require('./RollingFileWriteStream');

// just to adapt the previous version
class RollingFileStream extends RollingFileWriteStream {
  constructor(filename, size, backups, options) {
    if (!options) {
      options = {};
    }
    if (size) {
      options.maxSize = size;
    }
    if (!backups) {
      backups = 1;
    }
    options.numToKeep = backups;
    super(filename, options);
    this.backups = this.options.numToKeep;
    this.size = this.options.maxSize;
  }

  get theStream() {
    return this.currentFileStream;
  }

}

module.exports = RollingFileStream;
