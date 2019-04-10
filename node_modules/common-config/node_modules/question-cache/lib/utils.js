'use strict';

var utils = require('lazy-cache')(require);
var fn = require;

/**
 * Lazily required module dependencies
 */

require = utils;
require('arr-flatten', 'flatten');
require('arr-union', 'union');
require('async');
require('define-property', 'define');
require('get-value', 'get');
require('has-value', 'has');
require('inquirer2', 'inquirer');
require('is-answer');
require('isobject', 'isObject');
require('mixin-deep', 'merge');
require('omit-empty');
require('project-name', 'project');
require('set-value', 'set');
require('to-choices');
require = fn;

utils.decorate = function(opts) {
  utils.define(opts, 'set', function(prop, val) {
    utils.set(this, prop, val);
    return this;
  }.bind(opts));

  utils.define(opts, 'get', function(prop) {
    return utils.get(this, prop);
  }.bind(opts));

  utils.define(opts, 'enabled', function(prop) {
    return this.get(prop) === true;
  }.bind(opts));

  utils.define(opts, 'disabled', function(prop) {
    return this.get(prop) === false;
  }.bind(opts));
};

utils.isEmpty = function(answer) {
  return !utils.isAnswer(answer);
};

utils.matchesKey = function(prop, key) {
  if (typeof key !== 'string' || typeof prop !== 'string') {
    return false;
  }
  if (prop === key) {
    return true;
  }
  var len = prop.length;
  var ch = key.charAt(len);
  return key.indexOf(prop) === 0 && ch === '.';
};

/**
 * Cast val to an array
 */

utils.arrayify = function(val) {
  if (!val) return [];
  return Array.isArray(val) ? val : [val];
};

/**
 * Returns true if a value is an object and appears to be a
 * question object.
 */

utils.isQuestion = function(val) {
  return utils.isObject(val) && (val.isQuestion || !utils.isOptions(val));
};

/**
 * Returns true if a value is an object and appears to be an
 * options object.
 */

utils.isOptions = function(val) {
  if (!utils.isObject(val)) {
    return false;
  }
  if (val.hasOwnProperty('locale')) {
    return true;
  }
  if (val.hasOwnProperty('force')) {
    return true;
  }
  if (val.hasOwnProperty('type')) {
    return false;
  }
  if (val.hasOwnProperty('message')) {
    return false;
  }
  if (val.hasOwnProperty('choices')) {
    return false;
  }
  if (val.hasOwnProperty('name')) {
    return false;
  }
};

/**
 * Expose `utils`
 */

module.exports = utils;
