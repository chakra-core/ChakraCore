'use strict';

const Plugin = require('broccoli-plugin');
const fs = require('fs-extra');
const symlinkOrCopy = require('symlink-or-copy');

module.exports = class Noop extends Plugin {
  constructor(input, options) {
    super([input], {
      name: options.name,
      annotation: options.annotation || options.name,
      persistentOutput: true
    });
    this._linked = false;
    this.options = options;
  }

  build() {
    let input = this.inputPaths[0];

    this.options.build.apply(this, arguments);
    if (this._linked) { return }
    let output = this.outputPath;

    fs.rmdirSync(output);
    symlinkOrCopy.sync(input, output);
    this._linked = true;
  }
};
