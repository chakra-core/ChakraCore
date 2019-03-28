'use strict';

var utils = require('./utils');

var Data = module.exports = function Data() {};

Data.prototype.set = function() {
  utils.set.bind(this, this).apply(this, arguments);
  return this;
};

Data.prototype.get = function() {
  return utils.get.bind(this, this).apply(this, arguments);
};

Data.prototype.merge = function() {
  utils.merge.bind(this, this).apply(this, arguments);
  return this;
};
