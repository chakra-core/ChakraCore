const fs        = require('fs');
const path      = require('path');
const read      = fs.readFileSync;
const exists    = fs.existsSync;
const debug     = require('./debug')('tabtab:cache');
const cachefile = path.join(__dirname, '../.completions/cache.json');

// Cache mixin
module.exports = {
  initCache() {
    this._cache = exists(cachefile) ? require(cachefile) : { timestamp: Date.now(), cache: {} };
    this.cacheStore = this._cache.cache;
  },

  cache(key, value) {
    if (!this.cacheStore) this.initCache();
    if (!value) return this.fromStore(key);
    this.cacheStore[key] = { timestamp: Date.now(), value};
    // this.writeToStore(this.cacheStore);
  },

  fromStore(key) {
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

  writeToStore(cache) {
    debug('Writing to %s', cachefile);
    fs.writeFileSync(cachefile, JSON.stringify({
      timestamp: Date.now(),
      cache: cache
    }, null, 2));
  }
};
