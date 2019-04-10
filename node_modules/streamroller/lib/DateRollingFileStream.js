const RollingFileWriteStream = require('./RollingFileWriteStream');

// just to adapt the previous version
class DateRollingFileStream extends RollingFileWriteStream {
  constructor(filename, pattern, options) {
    if (pattern && typeof(pattern) === 'object') {
      options = pattern;
      pattern = null;
    }
    if (!options) {
      options = {};
    }
    if (!pattern) {
      pattern = 'yyyy-MM-dd';
    }
    if (options.daysToKeep) {
      options.numToKeep = options.daysToKeep;
    }
    if (pattern.startsWith('.')) {
      pattern = pattern.substring(1);
    }
    options.pattern = pattern;
    super(filename, options);
    this.mode = this.options.mode;
  }

  get theStream() {
    return this.currentFileStream;
  }

}

module.exports = DateRollingFileStream;
