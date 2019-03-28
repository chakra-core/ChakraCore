/*!
 * base-npm (https://github.com/jonschlinkert/base-npm)
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var path = require('path');
var isValid = require('is-valid-app');
var extend = require('extend-shallow');
var spawn = require('cross-spawn');
var request = require('min-request');
var reduce = require('async-array-reduce');

module.exports = function(options) {
  return function(app) {
    if (!isValid(app, 'base-npm')) return;

    /**
     * Execute `npm install` with the given `args`, package `names`
     * and callback.
     *
     * ```js
     * app.npm('--save', ['isobject'], function(err) {
     *   if (err) throw err;
     * });
     * ```
     * @name .npm
     * @param {String|Array} `args`
     * @param {String|Array} `names`
     * @param {Function} `cb` Callback
     * @api public
     */

    this.define('npm', npm);
    function npm(cmds, args, cb) {
      var cwd = app.cwd || process.cwd();
      args = arrayify(cmds).concat(arrayify(args));
      spawn('npm', args, {cwd: cwd, stdio: 'inherit'})
        .on('error', cb)
        .on('close', function(code, err) {
          cb(err, code);
        });
    };

    /**
     * Install one or more packages. Does not save anything to
     * package.json. Equivalent of `npm install foo`.
     *
     * ```js
     * app.npm.install('isobject', function(err) {
     *   if (err) throw err;
     * });
     * ```
     * @name .npm.install
     * @param {String|Array} `names` package names
     * @param {Function} `cb` Callback
     * @api public
     */

    npm.install = function(args, cb) {
      npm('install', args, cb);
    };

    /**
     * (Re-)install and save the latest version of all `dependencies`
     * and `devDependencies` currently listed in package.json.
     *
     * ```js
     * app.npm.latest(function(err) {
     *   if (err) throw err;
     * });
     * ```
     * @name .npm.latest
     * @param {Function} `cb` Callback
     * @api public
     */

    npm.latest = function(names, cb) {
      if (typeof names !== 'function') {
        npm.install(latest(names), cb);
        return;
      }

      if (typeof names === 'function') {
        cb = names;
      }

      var devDeps = latest(pkg(app, 'devDependencies'));
      var deps = latest(pkg(app, 'dependencies'));

      npm.dependencies(deps, function(err) {
        if (err) return cb(err);
        npm.devDependencies(devDeps, cb);
      });
    };

    /**
     * Execute `npm install --save` with one or more package `names`.
     * Updates `dependencies` in package.json.
     *
     * ```js
     * app.npm.dependencies('micromatch', function(err) {
     *   if (err) throw err;
     * });
     * ```
     * @name .npm.dependencies
     * @param {String|Array} `names`
     * @param {Function} `cb` Callback
     * @api public
     */

    npm.dependencies = function(names, cb) {
      var args = [].concat.apply([], [].slice.call(arguments));
      cb = args.pop();

      if (args.length === 0) {
        args = pkg(app, 'dependencies');
      }

      if (!args.length) {
        cb();
        return;
      }
      npm.install(['--save'].concat(args), cb);
    };

    /**
     * Execute `npm install --save-dev` with one or more package `names`.
     * Updates `devDependencies` in package.json.
     *
     * ```js
     * app.npm.devDependencies('isobject', function(err) {
     *   if (err) throw err;
     * });
     * ```
     * @name .npm.devDependencies
     * @param {String|Array} `names`
     * @param {Function} `cb` Callback
     * @api public
     */

    npm.devDependencies = function(names, cb) {
      var args = [].concat.apply([], [].slice.call(arguments));
      cb = args.pop();

      if (args.length === 0) {
        args = pkg(app, 'devDependencies');
      }

      if (!args.length) {
        cb();
        return;
      }
      npm.install(['--save-dev'].concat(args), cb);
    };

    /**
     * Execute `npm install --global` with one or more package `names`.
     *
     * ```js
     * app.npm.global('mocha', function(err) {
     *   if (err) throw err;
     * });
     * ```
     * @name .npm.global
     * @param  {String|Array} `names`
     * @param  {Function} `cb` Callback
     * @api public
     */
    npm.global = function(names, cb) {
      var args = [].concat.apply([], [].slice.call(arguments));
      cb = args.pop();

      if (!args.length) {
        cb();
        return;
      }
      npm.install(['--global'].concat(args), cb);
    };

    /**
     * Check if one or more names exist on npm.
     *
     * ```js
     * app.npm.exists('isobject', function(err, results) {
     *   if (err) throw err;
     *   console.log(results.isobject);
     * });
     * //=> true
     * ```
     * @name .npm.exists
     * @param {String|Array} `names`
     * @param {Function} `cb` Callback
     * @returns {Object} Object of results where the `key` is the name and the value is `true` or `false`.
     * @api public
     */

    npm.exists = function(names, cb) {
      var names = [].concat.apply([], [].slice.call(arguments));
      cb = names.pop();

      reduce(names, {}, function(acc, name, next) {
        checkName(name, function(err, exists) {
          if (err) return next(err);
          acc[name] = exists;
          next(null, acc);
        });
      }, cb);
    };

    /**
     * Aliases
     */

    npm.saveDev = npm.devDependencies;
    npm.save = npm.dependencies;
  };
};

/**
 * Get the package.json for the current project
 */

function pkg(app, prop) {
  return Object.keys(pkgData(app)[prop] || {});
}

function pkgPath(app) {
  return path.resolve(app.cwd || process.cwd(), 'package.json');
}

function pkgData(app) {
  return app.pkg ? app.pkg.data : require(pkgPath(app));
}

/**
 * Prefix package names with `@latest`
 */

function latest(keys) {
  if (typeof keys === 'string') {
    keys = [keys];
  }
  if (!Array.isArray(keys)) {
    return [];
  }
  return keys.map(function(key) {
    return key.charAt(0) !== '-' ? (key + '@latest') : key;
  });
}

/**
 * Cast `val` to an array
 */

function arrayify(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
}

/**
 * Check if a name exists on npm.
 *
 * ```js
 * checkName('foo', function(err, exists) {
 *   if (err) throw err;
 *   console.log(exists);
 * });
 * //=> true
 * ```
 * @param  {String} `name` Name of module to check.
 * @param  {Function} `cb` Callback function
 */

function checkName(name, cb) {
  request(`https://registry.npmjs.org/${name}/latest`, {}, function(err, res, msg) {
    if (err) return cb(err);

    if (typeof msg === 'string' && msg === 'Package not found') {
      cb(null, false);
      return;
    }

    if (res && res.statusCode === 404) {
      return cb(null, false);
    }

    cb(null, true);
  });
}
