/*!
 * match-file <https://github.com/jonschlinkert/match-file>
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var path = require('path');
var isGlob = require('is-glob');
var isObject = require('isobject');
var mm = require('micromatch');

function matchFile(name, file) {
  if (typeof name !== 'string') {
    throw new TypeError('expected name to be a string');
  }
  if (!isObject(file) || file._isVinyl !== true) {
    throw new TypeError('expected file to be an object');
  }

  return endsWith(file.history[0], name)
    || endsWith(file.path, name)
    || file.stem === name
    || file.key === name;
}

matchFile.matcher = function(pattern, options) {
  if (typeof pattern !== 'string') {
    throw new TypeError('expected pattern to be a string');
  }

  if (!isGlob(pattern)) {
    return function(file) {
      return matchFile(pattern, file);
    };
  }

  var isMatch = mm.matcher(pattern, options);
  return function(file) {
    if (typeof file === 'string') {
      return isMatch(file);
    }

    return matchFile(pattern, file)
      || isMatch(file.key)
      || isMatch(file.history[0])
      || isMatch(file.path)
      || isMatch(path.resolve(file.path))
      || isMatch(file.relative)
      || isMatch(file.basename)
      || isMatch(file.stem);
  };
};

matchFile.isMatch = function(patterns, file, options) {
  return matchFile.matcher(patterns, options)(file);
};

function endsWith(filepath, name) {
  if (name.slice(0, 2) === './') {
    name = name.slice(2);
  }

  var len = name.length;
  var isMatch = filepath.slice(-len) === name;
  if (isMatch) {
    var ch = filepath.slice(-(len + 1), -len);
    return ch === '' || ch === '/' || ch === '\\';
  }
  return false;
}

/**
 * Expose `matchFile`
 */

module.exports = matchFile;
