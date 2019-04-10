'use strict';

var typeOf = require('kind-of');

/**
 * Expose `utils`
 */

var utils = module.exports;

/**
 * Return true if `val` is an object and not an array.
 *
 * @param {any} `val`
 * @return {Boolean}
 */

utils.isObject = function isObject(val) {
  return typeOf(val) === 'object';
};

/**
 * Cast val to any array.
 *
 * @param {any} `val`
 * @return {Array}
 */

utils.arrayify = function arrayify(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

/**
 * Move `prop` from `val.options` to `val`
 *
 * @param {Object} val
 * @param {String} prop
 * @return {Object}
 */

utils.move = function move(val, prop) {
  if (val.hasOwnProperty(prop)) {
    return val;
  }
  var opts = val.options || {};
  if (opts.hasOwnProperty(prop)) {
    val[prop] = opts[prop];
    delete val.options[prop];
  }
  return val;
};

/**
 * "reserved" keys. These are keys that are necessary
 * to separate from config, so we don't arbitrarily try
 * to expand options keys into src-dest mappings.
 *
 * These are keys that are typically passed as options
 * to glob, globule, globby expand-config, etc.
 */

utils.taskKeys = [
  'options'
];

utils.targetKeys = [
  'files',
  'src',
  'dest'
];

utils.optsKeys = [
  'base',
  'cwd',
  'destBase',
  'expand',
  'ext',
  'extDot',
  'extend',
  'filter',
  'flatten',
  'glob',
  'mapDest',
  'parent',
  'process',
  'rename',
  'renameFn',
  'srcBase',
  'statType',
  'transform'
];

utils.all = utils.taskKeys
  .concat(utils.targetKeys)
  .concat(utils.optsKeys);

