'use strict';

var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('extend-shallow', 'extend');
require('file-contents', 'contents');
require('glob-parent', 'parent');
require('is-glob');
require('kind-of', 'typeOf');
require('matched', 'glob');
require('vinyl');
require = fn;

/**
 * Set the `file.key` used for caching views
 */

utils.renameKey = function(file, opts) {
  if (opts && typeof opts.renameKey === 'function') {
    return opts.renameKey(file.key, file);
  }
  return file.key || file.path;
};

/**
 * Normalize the content/contents properties before passing
 * to syncContents, the initial value is correct
 */

utils.normalizeContent = function(view) {
  if (typeof view.contents === 'string') {
    view.content = view.contents;
    view.contents = new Buffer(view.contents);

  } else if (typeof view.content === 'string') {
    view.contents = new Buffer(view.content);

  } else if (utils.isBuffer(view.contents)) {
    view.content = view.contents.toString();
  }
};

/**
 * Cast val to an array
 */

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

/**
 * Return true if `val` is a valid view
 */

utils.isView = function(val) {
  if (!utils.isObject(val)) {
    return false;
  }
  return val.isView
    || val.isItem
    || val.path
    || val.contents
    || val.content;
};

/**
 * Return true if `val` is an object
 */

utils.isObject = function(val) {
  return utils.typeOf(val) === 'object';
};

/**
 * Return true if `val` is a buffer
 */

utils.isBuffer = function(val) {
  return utils.typeOf(val) === 'buffer';
};

/**
 * Expose `utils`
 */

module.exports = utils;
