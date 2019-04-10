'use strict';
const chalk = require('chalk');
const objectValues = require('object-values');
const eachAsync = require('each-async');
const symbols = require('log-symbols');
const rules = require('./rules');

module.exports = function () {
  let errCount = 0;

  console.log('\n' + chalk.underline.blue('Yeoman Doctor'));
  console.log('Running sanity checks on your system\n');

  eachAsync(objectValues(rules), (rule, i, cb) => {
    rule.verify(err => {
      console.log((err ? symbols.error : symbols.success) + ' ' + rule.description);

      if (err) {
        errCount++;
        console.log(err);
      }

      cb();
    });
  }, () => {
    if (errCount === 0) {
      console.log(chalk.green('\nEverything looks all right!'));
    } else {
      console.log(chalk.red('\nFound potential issues on your machine :('));
    }
  });
};
