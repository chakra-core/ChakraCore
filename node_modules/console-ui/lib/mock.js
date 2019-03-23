'use strict';

const UI      = require('./');
const through = require('through');

module.exports = class MockUI extends UI {
  constructor(options) {
    super({
      inputStream: through(),
      outputStream: through((data) => {
        if (options && options.outputStream) {
          options.outputStream.push(data);
        }

        this.output += data;
      }),
      errorStream: through((data) => {
        this.errors += data;
      })
    });

    this.output = '';
    this.errors = '';
    this.errorReport = '';
    this.errorLog = options && options.errorLog || [];
  }

  clear() {
    this.output = '';
    this.errors = '';
    this.errorLog = [];
  }
}
