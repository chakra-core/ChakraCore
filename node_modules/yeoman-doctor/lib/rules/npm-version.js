'use strict';
const binVersionCheck = require('bin-version-check');
const message = require('../message');

exports.OLDEST_NPM_VERSION = '3.3.0';
exports.description = 'npm version';

const errors = {
  oldNpmVersion() {
    return message.get('npm-version', {
      isWin: process.platform === 'win32'
    });
  }
};
exports.errors = errors;

exports.verify = cb => {
  binVersionCheck('npm', `>=${exports.OLDEST_NPM_VERSION}`)
    .then(cb, () => cb(errors.oldNpmVersion()));
};
