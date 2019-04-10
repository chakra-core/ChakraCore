'use strict';

const env = require('./env');

module.exports = function deprecation(msg) {
  let error = new Error();
  let stack = error.stack.split(/\n/);
  let source = stack[3];

  console.warn(require('chalk').yellow('DEPRECATION: ' + msg + '\n' + source));
};
