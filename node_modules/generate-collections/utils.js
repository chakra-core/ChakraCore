'use strict';

var path = require('path');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('falsey');
require('extend-shallow', 'extend');
require('is-valid-app', 'isValid');
require('isobject', 'isObject');
require('parser-front-matter', 'parser');
require = fn;

utils.renameFile = function(app) {
  return function(file, next) {
    var dest = app.options.dest || app.cwd;
    file.base = dest;
    file.cwd = dest;

    var data = utils.extend({}, file.data);
    if (utils.isObject(data.rename)) {
      for (var key in data.rename) {
        if (data.rename.hasOwnProperty(key)) {
          file[key] = data.rename[key];
        }
      }
    }

    utils.stripPrefixes(file);
    file.path = path.join(file.base, file.basename);
    next();
  };
};

/**
 * strip prefixes from dotfile and config templates
 */

utils.stripPrefixes = function(file) {
  file.basename = file.basename.replace(/^_/, '.');
  file.basename = file.basename.replace(/^\$/, '');
};

/**
 * Expose `utils` modules
 */

module.exports = utils;
