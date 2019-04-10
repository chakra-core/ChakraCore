'use strict';

const GenerateTask = require('./generate-from-blueprint');

class DestroyTask extends GenerateTask {
  constructor(options) {
    super(options);
    this.blueprintFunction = 'uninstall';
  }
}

module.exports = DestroyTask;
