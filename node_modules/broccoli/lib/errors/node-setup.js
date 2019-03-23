'use strict';

const BuilderError = require('./builder');
const wrapPrimitiveErrors = require('../utils/wrap-primitive-errors');

module.exports = class NodeSetupError extends BuilderError {
  constructor(originalError, nodeWrapper) {
    if (nodeWrapper == null) {
      // Chai calls new NodeSetupError() :(
      super();
      return;
    }
    originalError = wrapPrimitiveErrors(originalError);
    const message =
      originalError.message +
      '\nat ' +
      nodeWrapper.label +
      nodeWrapper.formatInstantiationStackForTerminal();
    super(message);
    // The stack will have the original exception name, but that's OK
    this.stack = originalError.stack;
  }
};
