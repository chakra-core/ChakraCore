'use strict';

const fs = require('fs');
const path = require('path');
const Plugin = require('broccoli-plugin');
const symlinkOrCopy = require('symlink-or-copy');

// Some addons do their own compilation, which means the addon trees will
// be a mix of ES6 and AMD files. This plugin gives us a way to separate the
// files, as we don't want to double compile the AMD code.

// It has a very simple method of detecting AMD code, because we only care
// about babel output, which is pretty consistent.
class AmdFunnel extends Plugin {
  constructor(inputNode, options = {}) {
    super([inputNode], {
      persistentOutput: true,
      annotation: options.annotation
    });

    this._options = options;
  }

  build() {
    if (this._hasRan && symlinkOrCopy.canSymlink) {
      return;
    }

    let [inputPath] = this.inputPaths;
    let outputPath = this.outputPath;

    let isAmd;
    let isScoped;

    (function walk(dir) {
      let dirs = [];
      for (let file of fs.readdirSync(dir)) {
        let filePath = path.join(dir, file);
        let stats = fs.statSync(filePath);
        if (stats.isDirectory()) {
          dirs.push(filePath);
        } else {
          if (!file.endsWith('.js')) {
            continue;
          }
          let source = fs.readFileSync(filePath);
          isAmd = source.indexOf('define(') === 0;
          return true;
        }
      }
      if (isScoped) {
        return true;
      }
      isScoped = true;
      for (let dir of dirs) {
        if (walk(dir)) {
          return true;
        }
      }
    })(inputPath);

    if (!isAmd) {
      if (this._hasRan) {
        fs.unlinkSync(outputPath);
      } else {
        fs.rmdirSync(outputPath);
      }

      symlinkOrCopy.sync(inputPath, outputPath);
    } else if (this._options.callback) {
      this._options.callback();
    }

    this._hasRan = true;
  }
}

module.exports = AmdFunnel;
