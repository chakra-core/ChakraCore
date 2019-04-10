'use strict';

var path = require('path');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('arr-union', 'union');
require('define-property', 'define');
require('engine-base', 'engine');
require('isobject', 'isObject');
require('extend-shallow', 'merge');
require = fn;

/**
 * Arrayify the given value by casting it to an array.
 */

utils.arrayify = function(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
};

/**
 * Resolve the name of the engine to use, or the file
 * extension to use for identifying the engine.
 *
 * @param {Object} `view`
 * @return {String}
 */

utils.resolveEngine = function(view) {
  var engine = view.options.engine || view.locals.engine || view.data.engine;
  if (!engine) {
    engine = path.extname(view.path);
    view.data.ext = engine;
  }
  return engine;
};

/**
 * Resolve the layout to use for `view`. If `options.resolveLayout` is
 * a function, it will be used instead.
 *
 * @param {Object} `view`
 * @return {String|undefined} The name of the layout or `undefined`
 * @api public
 */

utils.resolveLayout = function(view) {
  var layout;
  if (view.options && typeof view.options.resolveLayout === 'function') {
    layout = view.options.resolveLayout(view);
  } else {
    if (typeof layout === 'undefined') {
      layout = view.data.layout;
    }
    if (typeof layout === 'undefined') {
      layout = view.locals.layout;
    }
    if (typeof layout === 'undefined') {
      layout = view.options.layout;
    }
    if (view.isPartial && layout === 'default') {
      layout = undefined;
    }
  }
  return layout;
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
