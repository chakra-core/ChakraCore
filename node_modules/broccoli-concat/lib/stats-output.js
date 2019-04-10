'use strict';

const Debug = require('broccoli-debug');

module.exports = class StatsOutput extends Debug {

  constructor(inputNode, options) {
    return super(inputNode, {
      label: options.name,
      baseDir: options.dir,
      force: true
    });
  }

  build() {
    this._shouldSync = this.isEnabled();
    super.build();
  }

  isEnabled() {
    return !!process.env.CONCAT_STATS;
  }
};
