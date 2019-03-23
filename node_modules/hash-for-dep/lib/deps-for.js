'use strict';
var pkg = require('./pkg');

/* @private
 *
 * Constructs a set of all dependencies recursively.
 *
 * @method depsFor
 * @param {String} name of package to assemble unique dependencies for
 * @param {String} dir (optional) path to begin resolving from
 * @param {Object} options (optional)
 * @param {Boolean} options.includeOptionalDeps (optional) should optionalDependencies be included
 * @return {Array} a unique set of all deps
 */
module.exports = function depsFor(name, dir, _options) {
  var options = _options ? _options : {};
  var dependencies = [];
  var visited = Object.create(null);

  (function again(name, dir, _options) {
    var options = _options ? _options : {};
    var thePackage = pkg(name, dir);

    if (thePackage === null) { return; }

    var key = thePackage.name + thePackage.version + thePackage.baseDir;

    if (visited[key]) { return; }
    visited[key] = true;

    dependencies.push(thePackage);

    var depsOfThePackage = [];

    if (options.includeOptionalDeps) {
      depsOfThePackage = Object.keys(thePackage.optionalDependencies || {});
    }

    depsOfThePackage = depsOfThePackage.concat(Object.keys(thePackage.dependencies || {}));

    return depsOfThePackage.forEach(function(dep) { again(dep, thePackage.baseDir, options); });
  }(name, dir, options));

  return dependencies;
};

