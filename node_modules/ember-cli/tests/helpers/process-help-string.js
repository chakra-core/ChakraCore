'use strict';

const chalk = require('chalk');

// eslint-disable-next-line node/no-unpublished-require
const stripAnsi = require('strip-ansi');

module.exports = function(helpString) {
  // currently windows
  if (chalk.supportsColor) {
    return helpString;
  }
  return stripAnsi(helpString);
};
