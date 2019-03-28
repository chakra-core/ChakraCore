/*!
 * resolve-file <https://github.com/doowb/resolve-file>
 *
 * Copyright (c) 2015, Brian Woodward, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var path = require('path');
var utils = require('./utils');

/**
 * Resolve the path to a file located in one of the following places:
 *
 *  - local to the current project (`'./index.js'`)
 *  - absolute (`'/usr/something.rc'`)
 *  - node module "main" file (`'cwd'`)
 *  - specific file inside a node module (`'cwd/LICENSE'`)
 *  - file located in user's home directory (`'~/.npmrc'`)
 *
 * ```js
 * var fp = resolve('./index.js')
 * //=> /path/to/resolve-file/index.js
 * ```
 *
 * @param  {String} `name` Filename to resolve
 * @param  {Object} `options` Additional options to specify `cwd`
 * @return {String} Resolved `filepath` if found
 * @api public
 */

function resolve(name, options) {
  var file = resolve.file(name, options);
  return file && file.path;
}

/**
 * Resolve the path to a file located in one of the following places:
 *
 *  - local to the current project (`'./index.js'`)
 *  - absolute (`'/usr/something.rc'`)
 *  - node module "main" file (`'cwd'`)
 *  - specific file inside a node module (`'cwd/LICENSE'`)
 *  - file located in user's home directory (`'~/.npmrc'`)
 *
 * ```js
 * var file = resolve.file('./index.js')
 * //=> {
 * //=>   cwd: '/path/to/resolve-file',
 * //=>   path: '/path/to/resolve-file/index.js'
 * //=> }
 * ```
 *
 * @param  {String} `name` Filename to resolve
 * @param  {Object} `options` Additional options to specify `cwd`
 * @return {Object} File object with resolved `path` if found.
 * @api public
 */

resolve.file = function(name, options) {
  var file = {};
  if (name && typeof name === 'object' && name.path) {
    file = name;
    name = file.path;
  }

  if (typeof options === 'function') {
    options = { resolve: options };
  }

  var opts = utils.extend({cwd: process.cwd()}, options);
  var first = name.charAt(0);

  file.cwd = opts.cwd;
  if (name.indexOf('npm:') === 0) {
    try {
      file.path = utils.resolve.sync(path.resolve(file.cwd, name.slice(4)));
      file.cwd = path.dirname(file.path);
    } catch (err) {
      throw new Error(`cannot resolve: '${name}'`);
    }
  } else {

    switch (first) {
      case '~':
        file.cwd = utils.home();
        file.path = utils.expandTilde(name);
        break;
      case '.':
      default: {
        file.path = path.resolve(file.cwd, name);
        break;
      }
    }
  }

  if (!utils.exists(file.path)) {
    try {
      if (/[\\\/]/.test(name)) {
        file.basename = path.basename(name);
        var dirname = path.dirname(name);
        file.name = path.basename(dirname);
        file.main = require.resolve(dirname);
        file.path = path.resolve(path.dirname(file.main), file.basename);
      }

      if (!utils.exists(file.path)) {
        file.path = utils.resolve.sync(name, {basedir: file.cwd});
        file.main = file.path;
      }
    } catch (err) {
      // account for "not found" errors from node require or `resolve` lib
      if (err.code !== 'MODULE_NOT_FOUND' && !/Cannot find module/.test(err.message)) {
        throw err;
      }
    };
  }

  if (utils.exists(file.path)) {
    return utils.decorate(file, opts.resolve);
  }
  return null;
};

/**
 * Export `resolve`
 * @type {Function}
 */

module.exports = resolve;
