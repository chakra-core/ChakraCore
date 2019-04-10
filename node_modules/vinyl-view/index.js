'use strict';

var Item = require('vinyl-item');
var utils = require('./utils');

/**
 * Expose `View`
 */

module.exports = View;

/**
 * Create an instance of `View`. Optionally pass a default object
 * to use.
 *
 * ```js
 * var view = new View({
 *   path: 'foo.html',
 *   contents: new Buffer('...')
 * });
 * ```
 * @param {Object} `view`
 * @api public
 */

function View(view) {
  if (!(this instanceof View)) {
    return new View(view);
  }

  Item.call(this, view);
  delete this.isItem;
  this.is('View');
}

/**
 * Inherit `Item`
 */

Item.extend(View);

/**
 * Synchronously compile a view.
 *
 * ```js
 * var view = page.compile();
 * view.fn({title: 'A'});
 * view.fn({title: 'B'});
 * view.fn({title: 'C'});
 * ```
 *
 * @param  {Object} `locals` Optionally pass locals to the engine.
 * @return {Object} `View` instance, for chaining.
 * @api public
 */

View.prototype.compile = function(settings) {
  this.fn = utils.engine.compile(this.content, settings);
  return this;
};

/**
 * Synchronously render templates in `view.content`.
 *
 * ```js
 * var view = new View({content: 'This is <%= title %>'});
 * view.renderSync({title: 'Home'});
 * console.log(view.content);
 * ```
 * @param  {Object} `locals` Optionally pass locals to the engine.
 * @return {Object} `View` instance, for chaining.
 * @api public
 */

View.prototype.renderSync = function(locals) {
  // if the view is not already compiled, do that first
  if (typeof this.fn !== 'function') {
    this.compile(locals);
  }

  var context = this.context(locals);
  context.path = this.path;

  this.contents = new Buffer(this.fn(context));
  delete this._context;
  return this;
};

/**
 * Asynchronously render templates in `view.content`.
 *
 * ```js
 * view.render({title: 'Home'}, function(err, res) {
 *   //=> view object with rendered `content`
 * });
 * ```
 * @param  {Object} `locals` Context to use for rendering templates.
 * @api public
 */

View.prototype.render = function(locals, cb) {
  if (typeof locals === 'function') {
    return this.render({}, locals);
  }
  try {
    cb(null, this.renderSync(locals));
  } catch (err) {
    cb(err);
  }
};

/**
 * Create a context object from `locals` and the `view.data` and `view.locals` objects.
 * The `view.data` property is typically created from front-matter, and `view.locals` is
 * used when a `new View()` is created.
 *
 * This method be overridden either by defining a custom `view.options.context` function
 * to customize context for a view instance, or static [View.context](#view-context) function to customize
 * context for all view instances.
 *
 * ```js
 * var page = new View({path: 'a/b/c.txt', locals: {a: 'b', c: 'd'}});
 * var ctx = page.context({a: 'z'});
 * console.log(ctx);
 * //=> {a: 'z', c: 'd'}
 * ```
 * @param  {Object} `locals` Optionally pass a locals object to merge onto the context.
 * @return {Object} Returns the context object.
 * @api public
 */

View.prototype.context = function(locals) {
  return View.context.apply(this, arguments);
};

/**
 * Returns true if the view is the given `viewType`. Returns `false` if no type is assigned.
 * When used with vinyl-collections, types are assigned by their respective collections.
 *
 * ```js
 * var view = new View({path: 'a/b/c.txt', viewType: 'partial'})
 * view.isType('partial');
 * ```
 * @param {String} `type` (`renderable`, `partial`, `layout`)
 * @api public
 */

View.prototype.isType = function(type) {
  return this.viewType.indexOf(type) !== -1;
};

/**
 * Set the `viewType` on the view
 */

utils.define(View.prototype, 'viewType', {
  configurable: true,
  set: function(viewType) {
    this.set('options.viewType', utils.union([], this.options.viewType, viewType));
  },
  get: function() {
    return utils.arrayify(this.options.viewType || 'renderable');
  }
});

/**
 * Return true if the viewType array has `renderable`
 */

utils.define(View.prototype, 'isRenderable', {
  configurable: true,
  get: function() {
    return this.isType('renderable');
  }
});

/**
 * Return true if the viewType array has `partial`
 */

utils.define(View.prototype, 'isPartial', {
  configurable: true,
  get: function() {
    return this.isType('partial');
  }
});

/**
 * Return true if the viewType array has `layout`
 */

utils.define(View.prototype, 'isLayout', {
  configurable: true,
  get: function() {
    return this.isType('layout');
  }
});

/**
 * Ensure that the `engine` property is set on a view.
 */

utils.define(View.prototype, 'engine', {
  configurable: true,
  set: function(engine) {
    this.set('cache.engine', engine);
  },
  get: function() {
    return this.cache.engine || Item.resolveEngine(this);
  }
});

/**
 * Ensure that the `layout` property is set on a view.
 */

utils.define(View.prototype, 'layout', {
  configurable: true,
  set: function(layout) {
    this.set('cache.layout', layout);
  },
  get: function() {
    if (typeof this.cache.layout !== 'undefined') {
      return this.cache.layout;
    }
    this.set('cache.layout', utils.resolveLayout(this));
    return this.cache.layout;
  }
});

/**
 * Define a custom static `View.context` function to override default `.context` behavior.
 * See the [context](#context) docs for more info.
 *
 * ```js
 * // custom context function
 * View.context = function(locals) {
 *   // `this` is the view being rendered
 *   return locals;
 * };
 * ```
 * @name View.context
 * @param {Object} `locals`
 * @return {Object}
 * @api public
 */

View.context = function(data, locals) {
  if (this._context) return this._context;

  if (typeof locals === 'undefined' || typeof data === 'function') {
    locals = data;
    data = {};
  }

  if (typeof locals === 'function') {
    return locals.call(this, data);
  }

  return utils.merge({}, this.locals, data, locals, this.data);
};

/**
 * Expose static `resolveEngine` method to allow overriding
 */

View.resolveEngine = Item.resolveEngine;
