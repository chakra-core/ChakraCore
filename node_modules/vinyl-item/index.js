'use strict';

var Base = require('base');
var Vinyl = require('vinyl');
var Stream = require('stream');
var utils = require('./utils');
var builtInFields = [
  '_contents', '_symlink', 'contents', 'stat', 'history', 'path',
  '_base', 'base', '_cwd', 'cwd',
];

/**
 * Expose `Item`
 */

module.exports = exports = Item;

/**
 * Create an instance of `Item`. Optionally pass a default object to use. See [vinyl][]
 * docs for API details and additional documentation.
 *
 * ```js
 * var item = new Item({
 *   path: 'foo.html',
 *   contents: new Buffer('...')
 * });
 * ```
 * @param {Object} `item`
 * @api public
 */

function Item(item) {
  if (!item) item = {};

  utils.syncContents(this, item.contents || item.content);
  this.cache = item.cache || {};
  this.options = item.options || {};
  this.locals = item.locals || {};
  this.data = item.data || {};

  this.define('contents', {
    configurable: true,
    enumerable: false,
    set: function(val) {
      utils.syncContents(this, val);
    },
    get: function() {
      return this._contents;
    }
  });

  var base = item.base;
  delete item.base;

  Vinyl.call(this, item);
  Base.call(this, item);
  delete this.base;
  this.base = base;

  for (var key in item) {
    var val = item[key];
    if (key === 'stat' && utils.isObject(val) && val.mode) {
      this.set(key, utils.cloneStats(val));
    } else if (val) {
      this.set(key, val);
    }
  }

  this.is('Item');
  this.use(utils.option());
  this.use(utils.plugin());
}

/**
 * Inherit `Base` and `Vinyl`
 */

Base.inherit(Item, Vinyl);
Base.extend(Item);

/**
 * Re-decorate Item methods after calling
 * vinyl's `.clone()` method.
 *
 * ```js
 * item.clone({deep: true}); // false by default
 * ```
 * @param  {Object} `options`
 * @return {Object} `item` Cloned instance
 */

Item.prototype.clone = function(opts) {
  var self = this;
  opts = opts || {};

  if (typeof opts === 'boolean') {
    opts = { deep: true };
  }

  opts.deep = opts.deep === true;
  opts.contents = opts.contents !== false;

  // clone the instance's item contents
  var contents = this.contents;
  if (this.isStream()) {
    contents = this.contents.pipe(new Stream.PassThrough());
    this.contents = this.contents.pipe(new Stream.PassThrough());
  } else if (this.isBuffer()) {
    contents = opts.contents ? utils.cloneBuffer(this.contents) : this.contents;
  }

  var item = new Item({
    cwd: this.cwd,
    base: this.base,
    stat: (this.stat ? utils.cloneStats(this.stat) : null),
    history: this.history.slice(),
    contents: contents
  });

  Object.keys(this).forEach(function(key) {
    if (self.constructor.isCustomProp(key)) {
      item[key] = opts.deep ? utils.clone(self[key], true) : self[key];
    }
  });

  if (opts.deep !== true) {
    item.options = this.options;
    item.data = this.data;
  } else {
    item.options = utils.extend({}, this.options);
    item.data = utils.extend({}, this.data);
  }
  return item;
};

Item.isCustomProp = function(key) {
  return builtInFields.indexOf(key) === -1;
};

/**
 * Override the vinyl `inspect` method.
 */

Item.prototype.inspect = function() {
  var name = this._name.charAt(0).toUpperCase() + this._name.slice(1);
  var inspect = [];

  var fp = this.key || ((this.path && this.base) ? this.relative : this.path);
  if (fp) inspect.push('"' + fp + '"');

  // if `isBuffer` isn't defined, we don't want this throwing an error,
  // we want the root cause to be transparent
  if (this.isBuffer && this.isBuffer()) {
    inspect.push(this.contents.inspect());
  }

  if (this.isStream && this.isStream()) {
    inspect.push(utils.inspectStream(this.contents));
  }
  return '<' + name + ' ' + inspect.join(' ') + '>';
};

Item.prototype.pipe = function(stream, options) {
  var opts = utils.extend({}, options);
  if (typeof opts.end === 'undefined') {
    opts.end = true;
  }

  if (this.isStream()) {
    return this.contents.pipe(stream, opts);
  }

  if (this.isBuffer()) {
    if (opts.end) {
      stream.end(this.contents);
    } else {
      stream.write(this.contents);
    }
    return stream;
  }

  if (opts.end) stream.end();
  return stream;
};

/**
 * Expose `filename` as an alias for `stem`
 */

utils.define(Item.prototype, 'filename', {
  configurable: true,
  set: function(val) {
    this.stem = val;
  },
  get: function() {
    return this.stem;
  }
});

/**
 * Normalize the `content` and `contents` properties on `item`. This is done to
 * ensure compatibility with the vinyl convention of using `contents` as a Buffer,
 * as well as the assemble convention of using `content` as a string. We will eventually
 * deprecate the `content` property.
 *
 * ```js
 * var item = new Item({path: 'foo/bar.hbs', contents: new Buffer('foo')});
 * console.log(item.content);
 * //=> 'foo'
 * ```
 * @name .content
 * @api public
 */

utils.define(Item.prototype, 'content', {
  configurable: true,
  set: function(val) {
    utils.syncContents(this, val);
  },
  get: function() {
    return this._content;
  }
});

/**
 * Getter/setter to resolve the name of the `engine` to use for rendering.
 *
 * ```js
 * var item = new Item({path: 'foo/bar.hbs'});
 * console.log(item.engine);
 * //=> '.hbs'
 * ```
 * @name .engine
 * @api public
 */

utils.define(Item.prototype, 'engine', {
  configurable: true,
  set: function(val) {
    this.define('_engine', val);
  },
  get: function() {
    return this._engine || Item.resolveEngine(this);
  }
});

/**
 * Expose static `resolveEngine` method to allow overriding
 */

Item.resolveEngine = utils.resolveEngine;
