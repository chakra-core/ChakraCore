'use strict';

var path = require('path');
var extend = require('extend-shallow');
var isValid = require('is-valid-app');
var isObject = require('isobject');

function middleware(app) {
  if (!isValid(app, 'middleware-rename-file')) return;
  app.onLoad(/./, renameFile(app));
  return middleware;
};

function renameFile(app) {
  return function(file, next) {
    if (file.isRenamed) {
      next(null, file);
      return;
    }

    file.isRenamed = true;
    file.rename = function(key, val) {
      return rename(file, key, val);
    };

    var data = extend({}, file.data);
    if (data.rename === false) {
      next(null, file);
      return;
    }

    if (isObject(data.rename)) {
      for (var key in data.rename) {
        rename(file, key, data.rename[key]);
      }
    }

    next(null, file);
  };
}

function rename(file, key, val) {
  switch (key) {
    case 'dirname':
      file.dirname = path.resolve(file.base, val);
      break;
    case 'relative':
      file.path = path.resolve(file.base, path.resolve(val));
      break;
    case 'basename':
      file.basename = val;
      break;
    case 'stem':
      file.stem = val;
      break;
    case 'extname':
      if (val && val.charAt(0) !== '.') {
        val = '.' + val;
      }
      file.extname = val;
      break;
    case 'path':
    default: {
      file.path = path.resolve(file.base, val);
      break;
    }
  }
}

/**
 * Expose middleware
 */

middleware.rename = rename;
middleware.renameFile = renameFile;
module.exports = middleware;
