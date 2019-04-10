/*!
 * middleware-utils <https://github.com/jonschlinkert/middleware-utils>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var each = require('async-each');
var reduce = require('async-array-reduce');

/**
 * Run one or more middleware in series.
 *
 * ```js
 * var utils = require('middleware-utils');

 * app.preRender(/\.hbs$/, utils.series([
 *   fn('foo'),
 *   fn('bar'),
 *   fn('baz')
 * ]));
 *
 * function fn(name) {
 *   return function(file, next) {
 *     console.log(name);
 *     next();
 *   };
 * }
 * ```
 * @param {Array|Function} `fns` Function or array of middleware functions
 * @api public
 */

exports.series = function(fns) {
  fns = arrayify.apply(null, arguments);
  return function(view, cb) {
    reduce(fns, view, function(file, fn, next) {
      // ensure errors are handled
      try {
        fn(file, function(err) {
          if (err) return next(err);
          next(null, file);
        });
      } catch (err) {
        next(err);
      }
    }, function(err) {
      if (err) return cb(err);
      cb(null, view);
    });
  };
};

/**
 * Run one or more middleware in parallel.
 *
 * ```js
 * var utils = require('middleware-utils');

 * app.preRender(/\.hbs$/, utils.parallel([
 *   fn('foo'),
 *   fn('bar'),
 *   fn('baz')
 * ]));
 *
 * function fn(name) {
 *   return function(file, next) {
 *     console.log(name);
 *     next();
 *   };
 * }
 * ```
 * @param {Array|Function} `fns` Function or array of middleware functions
 * @api public
 */

exports.parallel = function(fns) {
  fns = arrayify.apply(null, arguments);
  return function(file, cb) {
    each(fns, function(fn, next) {
      try {
        fn(file, function(err) {
          if (err) return next(err);
          next(null, file);
        });
      } catch (err) {
        next(err);
      }
    }, function(err) {
      if (err) return cb(err);
      cb(null, file);
    });
  };
};

/**
 * Format errors for the middleware `done` function. Takes the name of the middleware method
 * being handled.
 *
 * ```js
 * app.postRender(/./, function(view, next) {
 *   // do stuff to view
 *   next();
 * }, utils.error('postRender'));
 * ```
 *
 * @param {String} `method` The middleware method name
 * @api public
 */

exports.error = function(method) {
  return function(err, view, next) {
    next = next || function() {
      if (err) throw err;
    };

    if (err) {
      err.method = method;
      err.view = view;
      next(err);
      return;
    }
    next();
  };
};

/**
 * Format errors for the `app.handle()` method.
 *
 * ```js
 * app.handle('onFoo', view, utils.handleError(view, 'onFoo'));
 * ```
 *
 * @param {Object} `view` View object
 * @param {String} `method` The middleware method name
 * @param {String} `next` Callback function
 * @api public
 */

exports.handleError = function(view, method, next) {
  return function(err) {
    next = next || function() {
      if (err) throw err;
    };

    if (err) {
      err.method = method;
      err.view = view;
      next(err);
      return;
    }
    next();
  };
};

/**
 * Returns a function for escaping and unescaping erb-style template delimiters.
 *
 * ```js
 * var delims = mu.delims();
 * app.preRender(/\.tmpl$/, delims.escape());
 * app.postRender(/\.tmpl$/, delims.unescape());
 * ```
 * @param {Object} `options`
 * @api public
 */

exports.delims = function(options) {
  options = options || {};
  var escapeString = options.escapeString || '__UTILS_DELIM__';
  options.from = options.from || '{%%';
  options.to = options.to || '{%';
  var delims = {};

  // escape
  delims.escape = function(from) {
    from = from || options.from;
    return function(view, next) {
      view.content = view.content.split(from).join(escapeString);
      next();
    };
  };

  // unscape
  delims.unescape = function(to) {
    to = to || options.to;
    return function(view, next) {
      view.content = view.content.split(escapeString).join(to);
      next();
    };
  };
  return delims;
};

/**
 * Cast `val` to an array
 */

function arrayify(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
}
