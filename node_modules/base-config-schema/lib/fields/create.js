'use strict';

var debug = require('../debug');
var utils = require('../utils');

module.exports = function(app) {
  return function(val, key, options, schema) {
    if (!val || utils.isEmpty(val)) return null;
    debug.field(key, val);

    if (typeof val === 'string') {
      val = [val];
    }

    if (Array.isArray(val)) {
      var obj = {};
      val.forEach(function(name) {
        obj[name] = {cwd: app.cwd || process.cwd()};
      });
      val = obj;
    }
    return val;
  };
};
