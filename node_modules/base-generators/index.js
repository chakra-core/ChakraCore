/*!
 * base-generators <https://github.com/jonschlinkert/base-generators>
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var path = require('path');
var debug = require('debug')('base:generators');
var generator = require('./lib/generator');
var generate = require('./lib/generate');
var plugins = require('./lib/plugins');
var tasks = require('./lib/tasks');
var utils = require('./lib/utils');
var parseTasks = tasks.parse;

/**
 * Expose `generators`
 */

module.exports = function(config) {
  config = config || {};

  return function plugin(app) {
    if (!utils.isValid(app, 'base-generators')) return;

    this.isGenerator = true;
    this.generators = {};
    var cache = {};

    this.define('constructor', this.constructor);
    this.use(plugins());
    this.fns.push(plugin);

    /**
     * Add methods to `app` (instance of Base)
     */

    this.define({

      /**
       * Alias to `.setGenerator`.
       *
       * ```js
       * app.register('foo', function(app, base) {
       *   // "app" is a private instance created for the generator
       *   // "base" is a shared instance
       * });
       * ```
       * @name .register
       * @param {String} `name` The generator's name
       * @param {Object|Function|String} `options` or generator
       * @param {Object|Function|String} `generator` Generator function, instance or filepath.
       * @return {Object} Returns the generator instance.
       * @api public
       */

      register: function(name, options, generator) {
        return this.setGenerator.apply(this, arguments);
      },

      /**
       * Get and invoke generator `name`, or register generator `name` with
       * the given `val` and `options`, then invoke and return the generator
       * instance. This method differs from `.register`, which lazily invokes
       * generator functions when `.generate` is called.
       *
       * ```js
       * app.generator('foo', function(app, base, env, options) {
       *   // "app" - private instance created for generator "foo"
       *   // "base" - instance shared by all generators
       *   // "env" - environment object for the generator
       *   // "options" - options passed to the generator
       * });
       * ```
       * @name .generator
       * @param {String} `name`
       * @param {Function|Object} `fn` Generator function, instance or filepath.
       * @return {Object} Returns the generator instance or undefined if not resolved.
       * @api public
       */

      generator: function(name, val, options) {
        debug('.generator', name, val);

        if (this.hasGenerator(name)) {
          return this.getGenerator(name);
        }

        this.setGenerator.apply(this, arguments);
        return this.getGenerator(name);
      },

      /**
       * Store a generator by file path or instance with the given
       * `name` and `options`.
       *
       * ```js
       * app.setGenerator('foo', function(app, base) {
       *   // "app" - private instance created for generator "foo"
       *   // "base" - instance shared by all generators
       *   // "env" - environment object for the generator
       *   // "options" - options passed to the generator
       * });
       * ```
       * @name .setGenerator
       * @param {String} `name` The generator's name
       * @param {Object|Function|String} `options` or generator
       * @param {Object|Function|String} `generator` Generator function, instance or filepath.
       * @return {Object} Returns the generator instance.
       * @api public
       */

      setGenerator: function(name, val, options) {
        debug('.setGenerator', name);

        if (this.hasGenerator(name)) {
          return this.findGenerator(name);
        }

        // ensure local sub-generator paths are resolved
        if (typeof val === 'string' && val.charAt(0) === '.' && this.env) {
          val = path.resolve(this.env.dirname, val);
        }

        return generator(name, val, options, this);
      },

      /**
       * Get generator `name` from `app.generators`, same as [findGenerator], but also invokes
       * the returned generator with the current instance. Dot-notation may be used for getting
       * sub-generators.
       *
       * ```js
       * var foo = app.getGenerator('foo');
       *
       * // get a sub-generator
       * var baz = app.getGenerator('foo.bar.baz');
       * ```
       * @name .getGenerator
       * @param {String} `name` Generator name.
       * @return {Object|undefined} Returns the generator instance or undefined.
       * @api public
       */

      getGenerator: function(name, options) {
        debug('.getGenerator', name);

        if (name === 'this') {
          return this;
        }

        var gen = this.findGenerator(name, options);
        if (utils.isValidInstance(gen)) {
          return gen.invoke(gen, options);
        }
      },

      /**
       * Find generator `name`, by first searching the cache, then searching the
       * cache of the `base` generator. Use this to get a generator without invoking it.
       *
       * ```js
       * // search by "alias"
       * var foo = app.findGenerator('foo');
       *
       * // search by "full name"
       * var foo = app.findGenerator('generate-foo');
       * ```
       * @name .findGenerator
       * @param {String} `name`
       * @param {Function} `options` Optionally supply a rename function on `options.toAlias`
       * @return {Object|undefined} Returns the generator instance if found, or undefined.
       * @api public
       */

      findGenerator: function fn(name, options) {
        debug('.findGenerator', name);
        if (utils.isObject(name)) {
          return name;
        }

        if (Array.isArray(name)) {
          name = name.join('.');
        }

        if (typeof name !== 'string') {
          throw new TypeError('expected name to be a string');
        }

        if (cache.hasOwnProperty(name)) {
          return cache[name];
        }

        var app = this.generators[name]
          || this.base.generators[name]
          || this._findGenerator(name, options);

        // if no app, check the `base` instance
        if (typeof app === 'undefined' && this.hasOwnProperty('parent')) {
          app = this.base._findGenerator(name, options);
        }

        if (app) {
          cache[app.name] = app;
          cache[app.alias] = app;
          cache[name] = app;
          return app;
        }

        var search = {name, options};
        this.base.emit('unresolved', search, this);
        if (search.app) {
          cache[search.app.name] = search.app;
          cache[search.app.alias] = search.app;
          return search.app;
        }

        cache[name] = null;
      },

      /**
       * Private method used by `.findGenerator`.
       */

      _findGenerator: function(name, options) {
        if (this.generators.hasOwnProperty(name)) {
          return this.generators[name];
        }

        if (~name.indexOf('.')) {
          return this.getSubGenerator.apply(this, arguments);
        }

        var opts = utils.extend({}, this.options, options);
        if (typeof opts.lookup === 'function') {
          var app = this.lookupGenerator(name, opts, opts.lookup);
          if (app) {
            return app;
          }
        }
        return this.matchGenerator(name);
      },

      /**
       * Get sub-generator `name`, optionally using dot-notation for nested generators.
       *
       * ```js
       * app.getSubGenerator('foo.bar.baz');
       * ```
       * @name .getSubGenerator
       * @param {String} `name` The property-path of the generator to get
       * @param {Object} `options`
       * @api public
       */

      getSubGenerator: function(name, options) {
        debug('.getSubGenerator', name);
        var segs = name.split('.');
        var len = segs.length;
        var idx = -1;
        var app = this;

        while (++idx < len) {
          var key = segs[idx];
          app = app.getGenerator(key, options);
          if (!app) {
            break;
          }
        }
        return app;
      },

      /**
       * Iterate over `app.generators` and call `generator.isMatch(name)`
       * on `name` until a match is found.
       *
       * @param {String} `name`
       * @return {Object|undefined} Returns a generator object if a match is found.
       * @api public
       */

      matchGenerator: function(name) {
        debug('.matchGenerator', name);
        for (var key in this.generators) {
          var generator = this.generators[key];
          if (generator.isMatch(name)) {
            return generator;
          }
        }
      },

      /**
       * Returns true if the given generator `name` or `val` is already registered.
       *
       * ```js
       * console.log(app.hasGenerator('foo'));
       * ```
       * @param {String} `name`
       * @param {Object|Function} `val`
       * @return {Boolean}
       * @api public
       */

      hasGenerator: function(name, val) {
        return this.generators.hasOwnProperty(name) || this.base.generators[name];
      },

      /**
       * Tries to find a registered generator that matches `name`
       * by iterating over the `generators` object, and doing a strict
       * comparison of each name returned by the given lookup `fn`.
       * The lookup function receives `name` and must return an array
       * of names to use for the lookup.
       *
       * For example, if the lookup `name` is `foo`, the function might
       * return `["generator-foo", "foo"]`, to ensure that the lookup happens
       * in that order.
       *
       * @param {String} `name` Generator name to search for
       * @param {Object} `options`
       * @param {Function} `fn` Lookup function that must return an array of names.
       * @return {Object}
       * @api public
       */

      lookupGenerator: function(name, options, fn) {
        debug('.lookupGenerator', name);
        if (typeof options === 'function') {
          fn = options;
          options = {};
        }

        if (typeof fn !== 'function') {
          throw new TypeError('expected `fn` to be a lookup function');
        }

        options = options || {};

        // remove `lookup` fn from options to prevent self-recursion
        delete this.options.lookup;
        delete options.lookup;

        var names = fn(name);
        debug('looking up generator "%s" with "%j"', name, names);

        var len = names.length;
        var idx = -1;

        while (++idx < len) {
          var gen = this.findGenerator(names[idx], options);
          if (gen) {
            this.options.lookup = fn;
            return gen;
          }
        }

        this.options.lookup = fn;
      },

      /**
       * Extend the generator instance with settings and features
       * of another generator.
       *
       * ```js
       * var foo = base.generator('foo');
       * app.extendWith(foo);
       * // or
       * app.extendWith('foo');
       * // or
       * app.extendWith(['foo', 'bar', 'baz']);
       *
       * app.extendWith(require('generate-defaults'));
       * ```
       *
       * @name .extendWith
       * @param {String|Object} `app`
       * @return {Object} Returns the instance for chaining.
       * @api public
       */

      extendWith: function(name, options) {
        if (typeof name === 'function') {
          this.use(name, options);
          return this;
        }

        if (Array.isArray(name)) {
          var len = name.length;
          var idx = -1;
          while (++idx < len) {
            this.extendWith(name[idx], options);
          }
          return this;
        }

        var app = name;
        if (typeof name === 'string') {
          app = this.generators[name] || this.findGenerator(name, options);
          if (!app && utils.exists(name)) {
            var fn = utils.tryRequire(name, this.cwd);
            if (typeof fn === 'function') {
              app = this.register(name, fn);
            }
          }
        }

        if (!utils.isValidInstance(app)) {
          throw new Error('cannot find generator: "' + name + '"');
        }

        var alias = app.env ? app.env.alias : 'default';
        debug('extending "%s" with "%s"', alias, name);
        app.run(this);
        app.invoke(options, this);
        return this;
      },

      /**
       * Run a `generator` and `tasks`, calling the given `callback` function
       * upon completion.
       *
       * ```js
       * // run tasks `bar` and `baz` on generator `foo`
       * app.generate('foo', ['bar', 'baz'], function(err) {
       *   if (err) throw err;
       * });
       *
       * // or use shorthand
       * app.generate('foo:bar,baz', function(err) {
       *   if (err) throw err;
       * });
       *
       * // run the `default` task on generator `foo`
       * app.generate('foo', function(err) {
       *   if (err) throw err;
       * });
       *
       * // run the `default` task on the `default` generator, if defined
       * app.generate(function(err) {
       *   if (err) throw err;
       * });
       * ```
       * @name .generate
       * @emits `generate` with the generator `name` and the array of `tasks` that are queued to run.
       * @param {String} `name`
       * @param {String|Array} `tasks`
       * @param {Function} `cb` Callback function that exposes `err` as the only parameter.
       */

      generate: function(name, tasks, options, cb) {
        var self = this;

        if (typeof name === 'function') {
          return this.generate('default', [], {}, name);
        }
        if (utils.isObject(name)) {
          return this.generate('default', ['default'], name, tasks);
        }
        if (typeof tasks === 'function') {
          return this.generate(name, [], {}, tasks);
        }
        if (utils.isObject(tasks)) {
          return this.generate(name, [], tasks, options);
        }
        if (typeof options === 'function') {
          return this.generate(name, tasks, {}, options);
        }

        var results = [];
        if (Array.isArray(name)) {
          return utils.eachSeries(name, function(val, next) {
            self.generate(val, tasks, options, function(err, result) {
              if (err) return next(err);
              results = results.concat(result);
              next(null, result);
            });
          }, function(err) {
            if (err) return cb(err);
            cb(null, results);
          });
        }

        if (typeof cb !== 'function') {
          throw new TypeError('expected a callback function');
        }

        var queue = parseTasks(app, name, tasks);

        utils.eachSeries(queue, function(queued, next) {
          if (queued._ && queued._.length) {
            if (queued._[0] === 'default') {
              next();
              return;
            }
            var msg = utils.formatError('generator', app, queued._);
            next(new Error(msg));
            return;
          }

          if (cb.name === 'finishRun' && typeof name === 'string' && queued.tasks.indexOf(name) !== -1) {
            queued.name = name;
            queued.tasks = ['default'];
          }

          queued.generator = queued.app || this.getGenerator(queued.name, options);

          if (!utils.isGenerator(queued.generator)) {
            if (queued.name === 'default') {
              next();
              return;
            }
            next(new Error(utils.formatError('generator', app, queued.name)));
            return;
          }

          generate(this, queued, options, function(err, result) {
            if (err) return next(err);
            results = results.concat(result);
            next(null, result);
          });
        }.bind(this), function(err) {
          if (err) return cb(err);
          cb(null, results);
        });
        return;
      },

      /**
       * Create a generator alias from the given `name`. By default the alias
       * is the string after the last dash. Or the whole string if no dash exists.
       *
       * ```js
       * var camelcase = require('camel-case');
       * var alias = app.toAlias('foo-bar-baz');
       * //=> 'baz'
       *
       * // custom `toAlias` function
       * app.option('toAlias', function(name) {
       *   return camelcase(name);
       * });
       * var alias = app.toAlias('foo-bar-baz');
       * //=> 'fooBarBaz'
       * ```
       * @name .toAlias
       * @param {String} `name`
       * @param {Object} `options`
       * @return {String} Returns the alias.
       * @api public
       */

      toAlias: function(name, options) {
        if (typeof options === 'function') {
          return options(name);
        }
        if (options && typeof options.toAlias === 'function') {
          return options.toAlias(name);
        }
        if (typeof app.options.toAlias === 'function') {
          return app.options.toAlias(name);
        }
        return name;
      }
    });

    /**
     * Getter that returns `true` if the current instance is the `default` generator
     */

    Object.defineProperty(this, 'isDefault', {
      configurable: true,
      get: function() {
        return this.env && this.env.isDefault;
      }
    });

    return plugin;
  };
};

