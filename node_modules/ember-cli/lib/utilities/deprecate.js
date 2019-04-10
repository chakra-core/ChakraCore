'use strict';

const chalk = require('chalk');

module.exports = function(message, test) {
  if (test) {
    console.log(chalk.yellow(`DEPRECATION: ${message}`));
  }
};
