'use strict';

const Filter = require('broccoli-persistent-filter');
const minimatch = require('minimatch');
const debug = require('debug')('broccoli-stew:map');

/**
 * maps files, allow for simple content mutation
 *
 * @example
 *
 * const map = require('broccoli-stew').map;
 *
 * dist = map('lib', function(content, relativePath) {
 *   return 'new content';
 * });
 *
 * dist = map('lib', function(content, relativePath) {
 *   return 'prepend' + content;
 * });
 *
 * dist = map('lib', function(content, relativePath) {
 *   return mutateSomehow(content);
 * });
 *
 * dist = map('lib', '*.js', function(content, relativePath) {
 *   // mutate only files that match *.js
 *   // leave the rest alone
 *   return mutateSomehow(content);
 * });
 *
 */
module.exports = Mapper;
function Mapper(inputTree, _filter, _fn) {
  if (!(this instanceof Mapper)) {
    return new Mapper(inputTree, _filter, _fn);
  }
  Filter.call(this, inputTree);

  if (typeof _filter === 'function') {
    this.fn = _filter;
    this.filter = false;
  } else {
    this.filter = _filter;
    this.fn = _fn;
  }

  this._matches = Object.create(null);
}

Mapper.prototype = Object.create(Filter.prototype);
Mapper.prototype.constructor = Mapper;
Mapper.prototype.canProcessFile = function(relativePath) {
  if (this.filter) {
    let can = this.match(relativePath);
    debug('canProcessFile with filter: %s, %o', relativePath, can);
    return can;
  }

  debug('canProcessFile', relativePath);
  return true;
}

Mapper.prototype.getDestFilePath = function(path) {
  return path;
};

Mapper.prototype.match = function(path) {
  let cache = this._matches[path];
  if (cache === undefined) {
    return this._matches[path] = minimatch(path, this.filter);
  } else {
    return cache;
  }
};

Mapper.prototype.processString = function (string, relativePath) {
  debug('mapping: %s', relativePath);
  return this.fn(string, relativePath);
};
