'use strict';

function makeCache() {
  // object with no prototype
  const cache = Object.create(null);

  // force the jit to immediately realize this object is a dictionary. This
  // should prevent the JIT from going wastefully one direction (fast mode)
  // then going another (dict mode) after
  cache['_cache'] = 1;
  delete cache['_cache'];

  return cache;
}

export = class Cache {
  private _store: {[key: string]: string};
  constructor() {
    this._store = makeCache();

  }

  set(key: string, value: any) {
    return this._store[key] = value;
  }

  get(key: string) {
    return this._store[key];
  }

  has(key: string) {
    return key in this._store;
  }

  delete(key: string) {
    delete this._store[key];
  }

  get size() {
      return Object.keys(this._store).length;
  }
};
