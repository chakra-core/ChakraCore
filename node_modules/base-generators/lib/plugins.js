'use strict';

var utils = require('./utils');

module.exports = function() {
  return function fn(app) {
    if (utils.isValid(app, 'base-generators-plugins')) {
      this.define('isValid', utils.isValid);
      this.use(utils.plugins());
      this.use(utils.cwd());
      this.use(utils.pkg());
      this.use(utils.env());
      this.use(utils.option());
      this.use(utils.data());
      this.use(utils.compose());
      this.use(utils.task());
    }
    return fn;
  };
};
