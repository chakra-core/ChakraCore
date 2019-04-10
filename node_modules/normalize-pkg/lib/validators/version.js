'use strict';

var utils = require('../utils');

module.exports = function(val) {
  if (utils.semver.valid(val) === null) {
    throw new Error('invalid semver version "' + val + '" defined in package.json');
  }
  return true;
};
