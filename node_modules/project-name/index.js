/*!
 * project-name <https://github.com/jonschlinkert/project-name>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var fs = require('fs');
var path = require('path');
var find = require('find-pkg');
var repo = require('git-repo-name');

/**
 * Resolves in this order:
 *  1. package.json `name`
 *  2. git repository `name`
 *  3. `path.basename` of the current working directory
 */

module.exports = function(filepath) {
  return pkgname(filepath) || gitname(filepath) || basename(filepath);
};

/**
 * Get the `name` property from package.json
 */

function pkgname(filepath) {
  filepath = filepath || '';
  try {
    var pkgPath = find.sync(filepath, 0);
    if (!pkgPath) return null;
    var pkg = require(path.resolve(pkgPath));
    return pkg.name;
  } catch (err) {}
  return null;
}

/**
 * Get the git repository `name`, silently fail
 */

function gitname(filepath) {
  var dir = dirname(filepath || '');
  if (!dir) return null;

  try {
    return repo.sync(dir);
  } catch (err) {}
  return null;
}

/**
 * Get the `path.basename` of the current working directory.
 */

function basename(filepath) {
  var dir = dirname(filepath);
  if (!dir) return null;
  return path.basename(dir);
}

/**
 * Utility for getting the dirname of the given filepath.
 * The first result is cached to speed up subsequent
 * calls.
 */

function dirname(dir) {
  dir = path.resolve(dir || '');
  try {
    var stat = fs.statSync(dir);
    if (stat.isFile()) {
      dir = path.dirname(dir);
    }
    return path.basename(dir);
  } catch (err) {}
  return null;
}
