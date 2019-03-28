'use strict';

var flatten = require('arr-flatten');

module.exports = function arrayify(val) {
  val = Array.isArray(val) ? val : [val];
  return flatten(val).filter(Boolean);
};
