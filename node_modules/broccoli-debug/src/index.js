'use strict';
const fs = require('fs');
const path = require('path');
const symlinkOrCopy = require('symlink-or-copy');
const Plugin = require('broccoli-plugin');
const TreeSync = require('tree-sync');
const match = require('./match');
const buildDebugOutputPath = require('./util').buildDebugOutputPath;

module.exports = class BroccoliDebug extends Plugin {
  static buildDebugCallback(baseLabel) {
    return (input, labelOrOptions) => {
      // return input value if falsey
      if (!input) {
        return input;
      }

      let options = processOptions(labelOrOptions);
      options.label = `${baseLabel}:${options.label}`;

      if (options.force || match(options.label)) {
        return new this(input, options);
      }

      return input;
    };
  }

  constructor(node, labelOrOptions) {
    let options = processOptions(labelOrOptions);

    // this is used to avoid creating extraneous broccoli nodes when they are
    // not going to be used for debugging purposes (e.g. `BROCCOLI_DEBUG=*`
    // isn't present or doesn't apply to the current label)
    //
    // note: classes can only return an object or undefined from their
    // constructor, hence the guard for typeof object
    if (typeof node === 'object' && !options.force && !match(options.label)) {
      return node;
    }

    super([node], {
      name: 'BroccoliDebug',
      annotation: `DEBUG: ${options.label}`,
      persistentOutput: true,
      needsCache: false,
    });

    this.debugLabel = options.label;
    this._sync = undefined;
    this._haveLinked = false;
    this._debugOutputPath = buildDebugOutputPath(options);
    this._shouldSync = options.force || match(options.label);
  }

  build() {
    if (this._shouldSync) {
      let treeSync = this._sync;
      if (!treeSync) {
        treeSync = this._sync = new TreeSync(this.inputPaths[0], this._debugOutputPath);
      }

      treeSync.sync();
    }

    if (!this._haveLinked) {
      fs.rmdirSync(this.outputPath);
      symlinkOrCopy.sync(this.inputPaths[0], this.outputPath);
      this._haveLinked = true;
    }
  }
};

function processOptions(labelOrOptions) {
  let options = {
    baseDir: process.env.BROCCOLI_DEBUG_PATH || path.join(process.cwd(), 'DEBUG'),
    force: false
  };

  if (typeof labelOrOptions === 'string') {
    options.label = labelOrOptions;
  } else {
    Object.assign(options, labelOrOptions);
  }

  return options;
}

module.exports._shouldSyncDebugDir = match;
module.exports._buildDebugOutputPath = buildDebugOutputPath;
