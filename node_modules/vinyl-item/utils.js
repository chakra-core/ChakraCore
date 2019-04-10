'use strict';

var path = require('path');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('base-option', 'option');
require('base-plugins', 'plugin');
require('clone');
require('clone-stats');
require('define-property', 'define');
require('extend-shallow', 'extend');
require('isobject');
require = fn;

/**
 * Return true if the given value is an object.
 * @return {Boolean}
 */

utils.isObject = function(val) {
  return utils.isobject(val) || typeof val === 'function';
};

/**
 * Return true if `str` ends with substring `sub`
 */

utils.endsWith = function(str, sub) {
  return str.slice(-sub.length) === sub;
};

/**
 * Return true if the given value is a buffer
 */

utils.isBuffer = function(val) {
  if (val && val.constructor && typeof val.constructor.isBuffer === 'function') {
    return val.constructor.isBuffer(val);
  }
  return false;
};

/**
 * Return true if the given value is a stream.
 */

utils.isStream = function(val) {
  return utils.isObject(val)
    && (typeof val.pipe === 'function')
    && (typeof val.on === 'function');
};

/**
 * Resolve the name of the engine to use, or the file
 * extension to use for identifying the engine.
 *
 * @param {Object} `item`
 * @return {String}
 */

utils.resolveEngine = function(item) {
  var engine = item.options.engine || item.locals.engine || item.data.engine;
  if (!engine) {
    engine = path.extname(item.path);
    item.data.ext = engine;
  }
  return engine;
};

/**
 * Sync the _content and _contents properties on a view to ensure
 * both are set when setting either.
 *
 * @param {Object} `view` instance of a `View`
 * @param {String|Buffer|Stream|null} `contents` contents to set on both properties
 */

utils.syncContents = function(item, contents) {
  if (typeof item._contents === 'undefined') {
    item.define('_contents', null);
    item.define('_content', null);
  }
  if (contents === null) {
    item._contents = null;
    item._content = null;
  }
  if (typeof contents === 'string') {
    item._contents = new Buffer(contents);
    item._content = contents;
  }
  if (utils.isBuffer(contents)) {
    item._contents = contents;
    item._content = contents.toString();
  }
  if (utils.isStream(contents)) {
    item._contents = contents;
    item._content = contents;
  }
};

utils.inspectStream = function(stream) {
  var name = stream.constructor.name;
  if (!utils.endsWith(name, 'Stream')) {
    name += 'Stream';
  }
  return '<' + name + '>';
};

utils.cloneBuffer = function(buffer) {
  var res = new Buffer(buffer.length);
  buffer.copy(res);
  return res;
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
