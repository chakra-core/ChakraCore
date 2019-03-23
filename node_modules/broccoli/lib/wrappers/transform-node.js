'use strict';

const NodeWrapper = require('./node');
const fs = require('fs');
const undefinedToNull = require('../utils/undefined-to-null');
const rimraf = require('rimraf');

module.exports = class TransformNodeWrapper extends NodeWrapper {
  setup(features) {
    this.nodeInfo.setup(features, {
      inputPaths: this.inputPaths,
      outputPath: this.outputPath,
      cachePath: this.cachePath,
    });
    this.callbackObject = this.nodeInfo.getCallbackObject();
  }

  build() {
    let startTime;

    return new Promise(resolve => {
      startTime = process.hrtime();

      if (!this.nodeInfo.persistentOutput) {
        rimraf.sync(this.outputPath);
        fs.mkdirSync(this.outputPath);
      }

      resolve(this.callbackObject.build());
    }).then(() => {
      const now = process.hrtime();
      // Build time in milliseconds
      this.buildState.selfTime = 1000 * (now[0] - startTime[0] + (now[1] - startTime[1]) / 1e9);
      this.buildState.totalTime = this.buildState.selfTime;
      for (let i = 0; i < this.inputNodeWrappers.length; i++) {
        this.buildState.totalTime += this.inputNodeWrappers[i].buildState.totalTime;
      }
    });
  }

  toString() {
    let hint = this.label;
    if (this.inputNodeWrappers) {
      // a bit defensive to deal with partially-constructed node wrappers
      hint += ' inputNodeWrappers:[' + this.inputNodeWrappers.map(nw => nw.id) + ']';
    }
    hint += ' at ' + this.outputPath;
    if (this.buildState.selfTime != null) {
      hint += ' (' + Math.round(this.buildState.selfTime) + ' ms)';
    }
    return '[NodeWrapper:' + this.id + ' ' + hint + ']';
  }

  nodeInfoToJSON() {
    return undefinedToNull({
      nodeType: 'transform',
      name: this.nodeInfo.name,
      annotation: this.nodeInfo.annotation,
      persistentOutput: this.nodeInfo.persistentOutput,
      needsCache: this.nodeInfo.needsCache,
      // leave out instantiationStack (too long), inputNodes, and callbacks
    });
  }
};
