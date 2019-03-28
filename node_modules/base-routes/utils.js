'use strict';

var debug = require('debug')('base:routes');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('en-route', 'router');
require('is-valid-app');
require('template-error', 'rethrow');
require = fn;

utils.isValid = function(app) {
  if (!utils.isValidApp(app, 'base-routes', ['app', 'collection', 'views', 'list'])) {
    return false;
  }
  debug('loading routes methods');
  return true;
};

/**
 * Cast `val` to an array.
 */

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

utils.methods = [
  'onLoad',
  'preCompile',
  'preLayout',
  'onLayout',
  'postLayout',
  'onMerge',
  'onStream',
  'postCompile',
  'preRender',
  'postRender',
  'preWrite',
  'postWrite'
];

/**
 * Expose `utils` modules
 */

module.exports = utils;
