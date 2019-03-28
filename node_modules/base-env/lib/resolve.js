'use strict';

var fs = require('fs');
var path = require('path');
var utils = require('./utils');

/**
 * Return true if `filepath` exists on the file system
 */

module.exports = function(file, options) {
  var opts = utils.extend({}, options);

  if (utils.isAbsolute(file.path)) {
    file = utils.resolve.file(file, function(file) {
      return resolvePath(file, options);
    });

  } else if (typeof opts.cwd === 'string') {
    file = utils.resolve.file(file, opts);

  } else if (utils.exists(file.path)) {
    file = utils.resolve.file(file, opts);

  } else if (utils.tryResolve(file.path)) {
    file.path = utils.tryResolve(file.path);
    file = utils.resolve.file(file);

  } else if (!utils.isAbsolute(file.path)) {
    opts.cwd = utils.gm;
    file = utils.resolve.file(file, opts);
  }

  return file;
};

function resolvePath(file, options) {
  var opts = utils.extend({}, options, file.options);

  file.isDirectory = function() {
    if (!file.stat) {
      file.stat = fs.lstatSync(file.path);
    }
    return file.stat.isDirectory();
  };

  Object.defineProperty(file, 'pkgPath', {
    configurable: true,
    set: function(pkg) {
      this._pkgPath = pkg;
    },
    get: function() {
      if (utils.isString(this._pkgPath)) {
        return this._pkgPath;
      }
      var cwd = !this.isDirectory() ? this.dirname : this.path;
      var app = this.app || {};

      if (utils.isObject(app.pkg) && typeof app.pkg.get === 'function') {
        if (app.pkg.options.cwd === cwd) {
          return app.pkg.path;
        }
      }

      var pkgPath = path.resolve(cwd, 'package.json');
      if (utils.exists(pkgPath)) {
        return pkgPath;
      }
      return null;
    }
  });

  Object.defineProperty(file, 'pkg', {
    configurable: true,
    set: function(pkg) {
      this._pkg = pkg;
    },
    get: function() {
      if (utils.isObject(this._pkg) && utils.isString(this._pkg.name)) {
        return this._pkg;
      }

      if (this.pkgPath) {
        return (this._pkg = require(this.pkgPath));
      }
      return {};
    }
  });

  // do a quick check to see if `file.basename` has a dot. If not, then check to see
  // if `file.path` is a directory and if so attempt to resolve an actual file in
  // the directory
  if (file.isDirectory()) {
    var filepath = path.resolve(file.path, 'index.js');
    var basename = file.basename;

    if (file.pkg && !utils.exists(filepath)) {
      filepath = path.resolve(file.path, file.pkg.main);
    }

    if (utils.exists(filepath)) {
      file.folderName = basename;

      if (utils.isAbsolute(file.name)) {
        file.name = basename;
      }

      if (typeof file.options.toAlias === 'function') {
        file.alias = file.options.toAlias.call(file, file.name, file);
      }
      file.path = filepath;
      file.basename = path.basename(file.path);
      file.dirname = path.dirname(file.path);
    }
  }

  if (typeof opts.resolve === 'function') {
    // allow `file.path` to be updated or returned
    var res = opts.resolve(file);
    if (utils.isString(res)) {
      file.path = res;
    } else if (res) {
      file = res;
    }
  }
  return file;
};
