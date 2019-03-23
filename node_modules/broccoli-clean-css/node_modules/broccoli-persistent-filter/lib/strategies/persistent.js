'use strict';

var AsyncDiskCache = require('async-disk-cache');
var Promise = require('rsvp').Promise;

module.exports = {

  _cache: {},

  init: function(ctx) {
    if (!ctx.constructor._persistentCacheKey) {
      ctx.constructor._persistentCacheKey = this.cacheKey(ctx);
    }

    this._cache = new AsyncDiskCache(ctx.constructor._persistentCacheKey, {
      location: process.env['BROCCOLI_PERSISTENT_FILTER_CACHE_ROOT'],
      compression: 'deflate'
    });
  },

  cacheKey: function(ctx) {
    return ctx.cacheKey();
  },

  processString: function(ctx, contents, relativePath, instrumentation) {
    var key = ctx.cacheKeyProcessString(contents, relativePath);
    var cache = this._cache;
    var string;

    return cache.get(key).then(function(entry) {
      if (entry.isCached) {
        instrumentation.persistentCacheHit++;

        string = Promise.resolve(entry.value).then(function(value) {
          return JSON.parse(value);
        });
      } else {
        instrumentation.persistentCachePrime++;

        string = new Promise(function(resolve) {
          resolve(ctx.processString(contents, relativePath));
        }).then(function(result) {
          if (typeof result === 'string') {
            return { output: result };
          } else {
            return result;
          }
        }).then(function(value) {
          var stringValue = JSON.stringify(value);

          return cache.set(key, stringValue).then(function() {
            return value;
          });
        });
      }

      return string.then(function(object) {
        return new Promise(function(resolve) {
          resolve(ctx.postProcess(object, relativePath));
        }).then(function(result) {
          if (result === undefined) {
            throw new Error('You must return an object from `Filter.prototype.postProcess`.');
          }

          return result.output;
        });
      });
    });
  }
};
