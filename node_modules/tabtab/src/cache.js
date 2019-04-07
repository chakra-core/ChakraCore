'use strict';

var fs = require('fs');
var path = require('path');
var read = fs.readFileSync;
var exists = fs.existsSync;
var debug = require('./debug')('tabtab:cache');
var cachefile = path.join(__dirname, '../.completions/cache.json');

// Cache mixin
module.exports = {
  initCache: function initCache() {
    this._cache = exists(cachefile) ? require(cachefile) : { timestamp: Date.now(), cache: {} };
    this.cacheStore = this._cache.cache;
  },
  cache: function cache(key, value) {
    if (!this.cacheStore) this.initCache();
    if (!value) return this.fromStore(key);
    this.cacheStore[key] = { timestamp: Date.now(), value: value };
    // this.writeToStore(this.cacheStore);
  },
  fromStore: function fromStore(key) {
    var cache = this.cacheStore[key];
    if (!cache) return;

    var now = Date.now();
    var elapsed = now - cache.timestamp;
    if (elapsed > this.options.ttl) {
      debug('Cache TTL, invalid cache');
      delete this.cacheStore[key];
      this.writeToStore(this.cacheStore);
    }

    return cache;
  },
  writeToStore: function writeToStore(cache) {
    debug('Writing to %s', cachefile);
    fs.writeFileSync(cachefile, JSON.stringify({
      timestamp: Date.now(),
      cache: cache
    }, null, 2));
  }
};