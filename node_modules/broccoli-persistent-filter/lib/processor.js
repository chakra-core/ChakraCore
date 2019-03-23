'use strict';

module.exports = class Processor {
  constructor(options) {
    options = options || {};
    this.processor = {};
    this.persistent = options.persist;
  }

  setStrategy(stringProcessor) {
    this.processor = stringProcessor;
  }

  init(ctx) {
    this.processor.init(ctx);
  }

  processString(ctx, contents, relativePath, instrumentation) {
    return this.processor.processString(ctx, contents, relativePath, instrumentation);
  }
};
