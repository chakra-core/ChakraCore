'use strict';

module.exports = function(app, base, options) {
  return function(val, key, config, schema) {
    if (typeof val === 'undefined') {
      return;
    }

    if (typeof val === 'string') {
      var obj = {};
      obj[val] = true;
      val = obj;
    }

    return val;
  };
};
