'use strict';

const AsyncDiskCache = require('async-disk-cache');
const Promise = require('rsvp').Promise;

module.exports = {

  _cache: {},

  init(ctx) {
    if (!ctx.constructor._persistentCacheKey) {
      ctx.constructor._persistentCacheKey = this.cacheKey(ctx);
    }

    this._cache = new AsyncDiskCache(ctx.constructor._persistentCacheKey, {
      location: process.env['BROCCOLI_PERSISTENT_FILTER_CACHE_ROOT'],
      compression: 'deflate'
    });

    if (process.env['CLEAR_BROCCOLI_PERSISTENT_FILTER_CACHE'] === 'true') {
      this._cache.clear();
    }
  },

  cacheKey(ctx) {
    return ctx.cacheKey();
  },

  processString(ctx, contents, relativePath, instrumentation) {
    let key = ctx.cacheKeyProcessString(contents, relativePath);
    let cache = this._cache;
    let string;

    return cache.get(key).then(entry => {
      if (entry.isCached) {
        instrumentation.persistentCacheHit++;

        string = Promise.resolve(entry.value).then(value => {
          return JSON.parse(value);
        });
      } else {
        instrumentation.persistentCachePrime++;

        string = new Promise(resolve => {
          resolve(ctx.processString(contents, relativePath));
        }).then(result => {
          if (typeof result === 'string') {
            return { output: result };
          } else {
            return result;
          }
        }).then(value => {
          let stringValue = JSON.stringify(value);

          return cache.set(key, stringValue).then(() => value);
        });
      }

      return string.then(object => {
        return new Promise(resolve =>  {
          resolve(ctx.postProcess(object, relativePath));
        }).then(result => {
          if (result === undefined) {
            throw new Error('You must return an object from `Filter.prototype.postProcess`.');
          }

          return result.output;
        });
      });
    });
  }
};
