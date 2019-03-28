'use strict';

var debug = require('../debug');
var utils = require('../utils');

module.exports = function(app, prop) {
  return function(arr, key, config, next) {
    arr.forEach(function(name) {
      app.use(require(name));
    });
    next();
  }
};
