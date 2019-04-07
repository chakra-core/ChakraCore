'use strict';
function makeCache() {
    // object with no prototype
    var cache = Object.create(null);
    // force the jit to immediately realize this object is a dictionary. This
    // should prevent the JIT from going wastefully one direction (fast mode)
    // then going another (dict mode) after
    cache['_cache'] = 1;
    delete cache['_cache'];
    return cache;
}
module.exports = /** @class */ (function () {
    function Cache() {
        this._store = makeCache();
    }
    Cache.prototype.set = function (key, value) {
        return this._store[key] = value;
    };
    Cache.prototype.get = function (key) {
        return this._store[key];
    };
    Cache.prototype.has = function (key) {
        return key in this._store;
    };
    Cache.prototype.delete = function (key) {
        delete this._store[key];
    };
    Object.defineProperty(Cache.prototype, "size", {
        get: function () {
            return Object.keys(this._store).length;
        },
        enumerable: true,
        configurable: true
    });
    return Cache;
}());
