'use strict';
var customResolvePackagePath = require('./lib/resolve-package-path');
var ALLOWED_ERROR_CODES = {
    // resolve package error codes
    MODULE_NOT_FOUND: true,
    // Yarn PnP Error Codes
    UNDECLARED_DEPENDENCY: true,
    MISSING_PEER_DEPENDENCY: true,
    MISSING_DEPENDENCY: true
};
var CacheGroup = require("./lib/cache-group");
var getRealFilePath = customResolvePackagePath._getRealFilePath;
var getRealDirectoryPath = customResolvePackagePath._getRealDirectoryPath;
var CACHE = new CacheGroup();
var pnp;
try {
    pnp = require('pnpapi');
}
catch (error) {
    // not in Yarn PnP; not a problem
}
function resolvePackagePath(target, basedir, _cache) {
    var cache;
    if (_cache === undefined || _cache === null || _cache === true) {
        // if no cache specified, or if cache is true then use the global cache
        cache = CACHE;
    }
    else if (_cache === false) {
        // if cache is explicity false, create a throw-away cache;
        cache = new CacheGroup();
    }
    else {
        // otherwise, assume the user has provided an alternative cache for the following form:
        // provided by resolve-package-path/lib/cache-group.js
        cache = _cache;
    }
    var key = target + '\x00' + basedir;
    var pkgPath;
    if (cache.PATH.has(key)) {
        pkgPath = cache.PATH.get(key);
    }
    else {
        try {
            // the custom `pnp` code here can be removed when yarn 1.13 is the
            // current release. This is due to Yarn 1.13 and resolve interoperating
            // together seemlessly.
            pkgPath = pnp
                ? pnp.resolveToUnqualified(target + '/package.json', basedir)
                : customResolvePackagePath(cache, target, basedir);
        }
        catch (e) {
            if (e !== null && typeof e === 'object') {
                var code = e.code;
                if (ALLOWED_ERROR_CODES[code] === true) {
                    pkgPath = null;
                }
                else {
                    throw e;
                }
            }
            else {
                throw e;
            }
        }
        cache.PATH.set(key, pkgPath);
    }
    return pkgPath;
}
resolvePackagePath._resetCache = function () {
    CACHE = new CacheGroup();
};
(function (resolvePackagePath) {
})(resolvePackagePath || (resolvePackagePath = {}));
Object.defineProperty(resolvePackagePath, '_CACHE', {
    get: function () {
        return CACHE;
    }
});
resolvePackagePath.getRealFilePath = function (filePath) {
    return getRealFilePath(CACHE.REAL_FILE_PATH, filePath);
};
resolvePackagePath.getRealDirectoryPath = function (directoryhPath) {
    return getRealDirectoryPath(CACHE.REAL_DIRECTORY_PATH, directoryhPath);
};
module.exports = resolvePackagePath;
