/*!
 * base-fs-rename <https://github.com/jonschlinkert/base-fs-rename>
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var path = require('path');
var utils = require('./utils');

module.exports = function(config) {
  return function plugin(app) {
    if (!utils.isValid(app, 'base-fs-rename', ['app', 'collection'])) return;

    this.define('rename', function(dest, params) {
      if (typeof this.cwd !== 'string') {
        this.use(utils.cwd());
      }

      if (typeof dest !== 'string') {
        params = dest;
        dest = null;
      }

      return function(file) {
        var opts = utils.merge({}, app.options, config, params);
        opts.cwd = path.resolve(opts.dest || app.cwd);
        if (typeof dest === 'string') {
          opts.cwd = path.resolve(opts.cwd, dest);
        }

        file.cwd = opts.cwd;
        file.base = opts.cwd;

        normalizeDir(opts);
        normalizeExt(opts);

        for (var key in opts) {
          if (opts.hasOwnProperty(key) && (key in file)) {
            file[key] = opts[key];
          }
        }

        if (typeof params === 'string') {
          file.basename = params;
        }

        // replace leading non-word chars in the names of dotfile and config templates
        if (file.basename && opts.replace !== false) {
          file.basename = file.basename.replace(/^_/, '.').replace(/^\$/, '');
        }

        file.path = path.resolve(file.base, file.basename);
        return file.base;
      };
    });
    return plugin;
  };
};

function normalizeDir(opts) {
  var dir = opts.dir || opts.dirname;
  if (dir && !utils.isAbsolute(dir)) {
    opts.dirname = path.resolve(opts.cwd, dir);
  }
}

function normalizeExt(opts) {
  var ext = opts.extname || opts.ext;
  if (ext && ext.charAt(0) !== '.') {
    opts.extname = '.' + ext;
  }
}
