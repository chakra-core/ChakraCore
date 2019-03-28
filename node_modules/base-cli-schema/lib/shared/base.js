'use strict';

var path = require('path');
var utils = require('../utils');

module.exports = function(app) {
  return {
    type: ['array', 'boolean', 'string', 'object'],
    normalize: function(val, key, config, schema) {
      if (typeof val === 'undefined') {
        return;
      }
      schema.update('cwd', config);
      var cwd = path.resolve(config.cwd || app.cwd);

      if (utils.isObject(val)) {
        return val;
      }

      val = utils.arrayify(val);
      val = normalizeArray(cwd, val);
      config[key] = val;
      return val;
    }
  };
};

function normalizePath(cwd, name) {
  var fp = utils.stripQuotes(name);
  if (!fp || utils.hasGlob(fp)) {
    return fp;
  }
  if (fp.charAt(0) === '.' || /[\\\/]/.test(fp)) {
    fp = path.resolve(fp);
  }
  if (!utils.exists(fp)) {
    return name;
  }
  return path.resolve(cwd, fp);
}

function normalizeArray(cwd, arr) {
  var len = arr.length;
  var idx = -1;
  var res = [];

  while (++idx < len) {
    res = res.concat(normalizePath(cwd, arr[idx]));
  }
  return res;
}
