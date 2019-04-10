var ensurePosix = require('ensure-posix-path');
var hash = require('object-hash');

/**
 * Configures the module resolver. Empty options will reset to default settings
 * @typedef {Object} Options
 * @property {Bool} throwOnRootAccess whether to throw on
 * @property {String} moduleRoot to use for each of the resolved modules
 * @param  {Options} options
 * @returns {Function} configured module resolver
 */
function resolveModules(options) {
  options || (options = {});
  var throwOnRootAccess = true;
  var moduleRoot;

  if (options.throwOnRootAccess === false) {
    throwOnRootAccess = false;
  }

  if (typeof options.moduleRoot === 'string') {
    moduleRoot = options.moduleRoot;
  }

  /**
   * Module resolver for AMD transpiling (Babel)
   * @param  {String} _child child path of module to resolve
   * @param  {String} _name name path of module to resolve
   * @returns {String} resolved module path
   */
  function moduleResolve(_child, _name) {
    var child = ensurePosix(_child);
    var name = ensurePosix(_name);
    if (child.charAt(0) !== '.') {
      return child;
    }

    var parts = child.split('/');
    var nameParts = name.split('/');
    var parentBase = nameParts.slice(0, -1);
    // Add moduleRoot if not already present
    if (moduleRoot && nameParts[0] !== moduleRoot) {
      parentBase = moduleRoot.split('/').concat(parentBase);
    }

    for (var i = 0, l = parts.length; i < l; i++) {
      var part = parts[i];

      if (part === '..') {
        if (parentBase.length === 0) {
          if (throwOnRootAccess) {
            throw new Error('Cannot access parent module of root');
          } else {
            continue;
          }
        }
        parentBase.pop();
      } else if (part === '.') {
        continue;
      } else {
        parentBase.push(part);
      }
    }

    return parentBase.join('/');
  }

  // broccoli-babel-transpiler caching

  moduleResolve.cacheKey = function() {
    return hash(options);
  };

  moduleResolve.baseDir = function() {
    return __dirname;
  };

  // parallel API - enable parallel babel transpilation for the moduleResolve function
  moduleResolve._parallelBabel = {
    requireFile: __filename,
    buildUsing: 'resolveModules',
    params: options
  };

  return moduleResolve;
}

module.exports = {
  moduleResolve: resolveModules(),
  resolveModules: resolveModules
};
