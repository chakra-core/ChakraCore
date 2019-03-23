'use strict';

var resolvePkg = require('./resolve-pkg.js');

/* @private
 *
 * given the name of a descendent module this module locates and returns its
 * package.json. In addition, it provides the baseDir.
 *
 * @method pkg
 * @param {String} name
 * @param {String} dir (optional) root directory to begin resolution
 */
module.exports = function pkg(name, dir) {
  if (name !== './') { name += '/'; }

  var packagePath = resolvePkg(name, dir);
  if (packagePath === null) { return null; }

  var thePackage = require(packagePath);

  thePackage.baseDir = packagePath.slice(0, packagePath.length - 12 /* index of `/package.json` */);

  return thePackage;
};
