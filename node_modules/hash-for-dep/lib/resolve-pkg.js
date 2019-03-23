'use strict';
var resolve = require('resolve');
var resolvePackagePath = require('resolve-package-path');

var path = require('path');

/* @private
 *
 * @method resolvePkg
 * @param {String} name
 * @param {String} dir
 * @return {String}
 */
module.exports = function resolvePkg(name, dir) {
  if (name.charAt(0) === '/') {
    return name + '/package.json';
  }

  if (name === './') {
    return path.resolve(name, 'package.json');
  }

  return resolvePackagePath(name, dir || __dirname);
};
