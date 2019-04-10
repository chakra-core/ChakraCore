'use strict';
const semver = require('semver');
const message = require('../message');

const OLDEST_NODE_VERSION = '4.2.0';

exports.description = 'Node.js version';

const errors = {
  oldNodeVersion() {
    return message.get('node-version');
  }
};
exports.errors = errors;

exports.verify = cb => {
  cb(semver.lt(process.version, OLDEST_NODE_VERSION) ? errors.oldNodeVersion() : null);
};
