'use strict';

const undefinedToNull = require('../utils/undefined-to-null');

module.exports = class NodeWrapper {
  constructor() {
    this.buildState = {};
  }

  toJSON() {
    return undefinedToNull({
      id: this.id,
      nodeInfo: this.nodeInfoToJSON(),
      buildState: this.buildState,
      label: this.label,
      inputNodeWrappers: this.inputNodeWrappers.map(nw => nw.id),
      cachePath: this.cachePath,
      outputPath: this.outputPath,
      // leave out node, originalNode, inputPaths (redundant), build
    });
  }

  formatInstantiationStackForTerminal() {
    return '\n-~- created here: -~-\n' + this.nodeInfo.instantiationStack + '\n-~- (end) -~-';
  }
};
