'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Module dependencies
 */

require('kind-of', 'typeOf');
require('arr-flatten', 'flatten');
require('extend-shallow', 'extend');
require('path-to-regexp', 'toRegex');
require = fn;

/**
 * Utis
 */

utils.arrayify = function arrayify(val) {
  return (val ? (Array.isArray(val) ? val : [val]) : []);
};

/**
 * Decode param value.
 *
 * @param {string} val
 * @return {string}
 * @api private
 */

utils.decodeParam = function decodeParam(val) {
  if (typeof val !== 'string') {
    return val;
  }

  try {
    return decodeURIComponent(val);
  } catch (e) {
    var err = new TypeError("Failed to decode param '" + val + "'");
    err.status = 400;
    throw err;
  }
};

/**
 * Expose utils
 */

module.exports = utils;
