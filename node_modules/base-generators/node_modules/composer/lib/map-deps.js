'use strict';

/**
 * Unique count of anonymous tasks.
 * @type {Number}
 */

var anonymousCount = 0;

/**
 * Use in a map function to register anonymous functions
 * when not registered already.
 *
 * ```js
 * // bind the composer to the mapDeps call
 * deps = deps.map(mapDeps.bind(this));
 * ```
 *
 * @param  {String|Function} `dep` Dependency name or anonymous function to be registered.
 * @return {String} Returns the dependency name
 */

module.exports = function(dep) {
  if (typeof dep === 'function') {
    var depName = dep.taskName || dep.name || '[anonymous (' + (++anonymousCount) + ')]';
    this.task(depName, dep);
    return depName;
  }
  return dep;
};
