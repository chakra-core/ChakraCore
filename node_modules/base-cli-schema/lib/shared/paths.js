'use strict';

var path = require('path');
var util = require('util');
var utils = require('../utils');

module.exports = function(command, val, config, key) {
  if (typeof val === 'undefined') {
    return;
  }
  // fix paths mistaken for dot-notation
  if (utils.isObject(val)) {
    val = utils.stringify(val);
  }

  if (typeof val === 'string') {
    return path.resolve(val);
  }

  if (typeof val === 'boolean') {
    val = config[key] = { show: true };
    return val;
  }

  val = util.inspect(val);
  throw new TypeError('--' + command + ': expected a string or boolean, but received: ' + val);
};
