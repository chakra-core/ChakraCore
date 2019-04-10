'use strict';

module.exports = function(app, options) {
  return function(val, key, config, schema) {
    if (typeof val === 'undefined') {
      return;
    }
    if (typeof val === 'boolean') {
      val = { render: val };
    }
    if (val === 'render') {
      val = { render: true };
    }
    return val;
  };
};
