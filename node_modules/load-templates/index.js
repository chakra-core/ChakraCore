/*!
 * load-templates <https://github.com/jonschlinkert/load-templates>
 *
 * Copyright (c) 2015, 2017, Jon Schlinkert.
 * Released under the MIT License.
 */

'use strict';

var path = require('path');
var File = require('vinyl');
var glob = require('matched');
var extend = require('extend-shallow');
var utils = require('./utils');

/**
 * Create an instance of `Loader` with the given `options`.
 *
 * ```js
 * var Loader = require('load-templates');
 * var loader = new Loader();
 * ```
 * @param {Object} `options`
 * @api public
 */

function Loader(options) {
  if (!(this instanceof Loader)) {
    return new Loader(options);
  }
  this.options = extend({}, options);
  this.cache = this.options.cache || {};
}

/**
 * Load one or more templates from a filepath, glob pattern, object, or
 * array of filepaths, glob patterns or objects. This method detects
 * the type of value to be handled then calls one of the other methods
 * to do the actual loading.
 *
 * ```js
 * var loader = new Loader();
 * console.log(loader.load(['foo/*.hbs', 'bar/*.hbs']));
 * console.log(loader.load({path: 'a/b/c.md'}));
 * console.log(loader.load('index', {path: 'a/b/c.md'}));
 * ```
 * @param {Object} `templates`
 * @param {Object} `options`
 * @return {Object} Returns the views from `loader.cache`
 * @api public
 */

Loader.prototype.load = function(templates, options) {
  switch (utils.typeOf(templates)) {
    case 'object':
      return this.addViews(templates, options);
    case 'array':
      for (let i = 0; i < templates.length; i++) {
        let val = templates[i];
        if (utils.isView(val)) {
          this.addView(val, options);
        } else {
          this.load(val, options);
        }
      }
      break;
    case 'string':
    default: {
      if (utils.isView(options)) {
        options.path = templates;
        utils.normalizeContent(options);
        return this.addView(options);
      }
      return this.globViews(templates, options);
    }
  }
  return this.cache;
};

/**
 * Create a `view` object from the given `template`. View objects are
 * instances of [vinyl][].
 *
 * ```js
 * console.log(loader.createView('test/fixtures/foo/bar.hbs'));
 * console.log(loader.createView('foo/bar.hbs', {cwd: 'test/fixtures'}));
 * ```
 * @param {Object|String} `template` Filepath or object with `path` or `contents` properties.
 * @param {Object} `options`
 * @return {Object} Returns the view.
 * @api public
 */

Loader.prototype.createView = function(template, options) {
  var opts = utils.extend({cwd: process.cwd()}, this.options, options);
  var view;

  if (utils.isObject(template)) {
    utils.normalizeContent(template);
    view = new File(template);
  } else {
    view = new File({path: path.resolve(opts.cwd, template)});
  }

  // set base with the glob parent, if available
  view.base = opts.base || path.resolve(opts.cwd, opts.parent || '');
  view.cwd = opts.cwd;

  // prime the view's metadata objects
  view.options = view.options || {};
  view.locals = view.locals || {};
  view.data = view.data || {};

  // temporarily set `key` before calling `renameKey`
  view.key = view.key || view.path;
  var key = utils.renameKey(view, opts);
  if (typeof key === 'string') {
    view.key = key;
  }

  utils.contents.sync(view, opts);

  if (typeof this.options.loaderFn === 'function') {
    view = this.options.loaderFn(view) || view;
  }

  return view;
};

/**
 * Create a view from the given `template` and cache it on `loader.cache`.
 *
 * ```js
 * var loader = new Loader();
 * loader.addView('foo.hbs');
 * console.log(loader.cache);
 * ```
 * @param {String|Object} `template`
 * @param {Object} `options`
 * @return {Object} Returns the `Loader` instance for chaining
 * @api public
 */

Loader.prototype.addView = function(template, options) {
  var view = this.createView(template, options);
  this.cache[view.key] = view;
  return this;
};

/**
 * Create from an array or object of `templates` and cache them on
 * `loader.cache`.
 *
 * ```js
 * var loader = new Loader();
 * loader.addViews([
 *   {path: 'test/fixtures/a.md'},
 *   {path: 'test/fixtures/b.md'},
 *   {path: 'test/fixtures/c.md'},
 * ]);
 * loader.addViews({
 *   d: {path: 'test/fixtures/d.md'},
 *   e: {path: 'test/fixtures/e.md'},
 *   f: {path: 'test/fixtures/f.md'},
 * });
 * loader.addViews([{
 *   g: {path: 'test/fixtures/g.md'},
 *   h: {path: 'test/fixtures/h.md'},
 *   i: {path: 'test/fixtures/i.md'},
 * }]);
 * console.log(loader.cache);
 * ```
 *
 * @param {Object} `templates`
 * @param {Object} `options`
 * @api public
 */

Loader.prototype.addViews = function(templates, options) {
  if (typeof templates === 'string' && utils.isView(options)) {
    let view = options;
    let key = templates;
    view.path = view.path || key;
    view.key = key;
    return this.addView(view);
  }

  if (Array.isArray(templates)) {
    for (let i = 0; i < templates.length; i++) {
      this.addView(templates[i], options);
    }

  } else if (utils.isObject(templates)) {
    for (let key in templates) {
      let view = templates[key];

      if (templates.hasOwnProperty(key)) {
        if (utils.isView(view)) {
          view.path = view.path || key;
          view.key = key;

        } else if (typeof view === 'string') {
          view = { content: view, path: key };
        }
      }
      this.addView(view, options);
    }
  }
  return this;
};

/**
 * Load templates from one or more glob `patterns` with the given `options`,
 * then cache them on `loader.cache`.
 *
 * ```js
 * var loader = new Loader();
 * var views = loader.globViews('*.hbs', {cwd: 'templates'});
 * ```
 * @param {String|Array} `patterns`
 * @param {Object} `options`
 * @return {Object} Returns `loader.cache`
 * @api public
 */

Loader.prototype.globViews = function(patterns, options) {
  let opts = extend({cwd: process.cwd()}, this.options, options);
  // don't support nonull, it doesn't make sense here
  delete opts.nonull;
  opts.cwd = path.resolve(opts.cwd);
  patterns = utils.arrayify(patterns);
  let len = patterns.length;
  let idx = -1;

  // iterate over all patterns, so we can get the actual glob parent
  while (++idx < len) {
    let pattern = patterns[idx];
    let isGlob = utils.isGlob(pattern);
    let files = isGlob ? glob.sync(pattern, opts) : [pattern];

    if (!files.length) continue;

    // get the glob parent to use as `file.base`
    let parent = isGlob ? utils.parent(pattern) : '';

    // create a view
    this.addViews(files, extend({}, opts, {parent: parent}));
  }
  return this.cache;
};

/**
 * Expose `Loader`
 */

module.exports = Loader;
