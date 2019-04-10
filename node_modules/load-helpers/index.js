/*!
 * load-helpers <https://github.com/jonschlinkert/load-helpers>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var Emitter = require('component-emitter');
var utils = require('./utils');

module.exports = loadHelpers;

function loadHelpers(cache, options) {
  options = options || {};
  cache = cache || {};

  function loaderFn(key, val) {
    if (typeof key === 'function') {
      throw new TypeError('key should be an object, array or string.');
    }

    if (typeof val === 'function' || typeof val === 'string') {
      return addHelper(key, val);
    }

    if (utils.isObject(key)) {
      return addHelpers(key, val);
    }

    val = val || {};
    key = utils.arrayify(key);
    if (!utils.isGlob(key)) {
      return addHelpers(key, val);
    }

    loader(key, val);
    return cache;
  }

  function addHelper(name, fn) {
    if (typeof fn === 'string') {
      fn = utils.tryRequire(fn);
    }

    if (typeof fn !== 'function' && !utils.isObject(fn)) {
      throw new TypeError('expected a function or object');
    }

    if (options.async === true) {
      fn.async = true;
    }

    cache[name] = fn;
    loaderFn.emit(options.emit || 'helper', name, fn, !!fn.async);
    return cache;
  }

  function addHelpers(helpers, opts) {
    if (Array.isArray(helpers) && helpers.length) {
      helpers.forEach(function(helper) {
        loaderFn(helper, opts);
      });
    } else if (utils.isObject(helpers)) {
      for (var name in helpers) {
        var helper = helpers[name];
        if (typeof helper === 'function') {
          addHelper(name, helper);
        } else if (utils.isObject(helper)) {
          addHelpers(helper, opts);
        } else if (typeof helper === 'string') {
          addHelper(name, utils.tryRequire(helper));
        } else if (Array.isArray(helper)) {
          var obj = {};
          obj[name] = {};
          helper.forEach(function(val) {
            utils.extend(obj[name], loaderFn(val, opts));
          });
          utils.extend(cache, obj);
        }
      }
    }
    return cache;
  }

  function loader(patterns, opts) {
    var files = utils.glob.sync(patterns, opts);
    var len = files.length;

    if (!len) {
      files = patterns;
      len = files.length;
    }

    while (len--) {
      var name = files[len];
      var file = utils.tryRequire(name, opts);
      if (typeof file === 'function') {
        name = utils.renameKey(name, opts);
        loaderFn(name, file);
      } else {
        loaderFn(file, opts);
      }
    }
    return cache;
  }

  return Emitter(loaderFn);
};
