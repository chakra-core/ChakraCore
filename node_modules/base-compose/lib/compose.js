'use strict';

var utils = require('./utils');

/**
 * Create a new instance of the Compose for the supplied provider and receiver instances,
 * with the list of given generators.
 *
 * ```js
 * var compose = new Compose(provider, receiver, ['a', 'b', 'c']);
 * ```
 * @param {Object} `provider` The generator instance with generators to extend onto the receiver instance.
 * @param {Object} `receiver` The current generator instance to extend.
 * @param {Array} `generators` One or more generators from the `provider` generator to iterate over.
 */

function Compose(provider, receiver, generators) {
  this.generators = utils.arrayify(generators);
  this.provider = provider;
  this.receiver = receiver;
}

/**
 * Merge the options from each generator into the `app` options.
 * This method requires using the [base-option][base-option] plugin.
 *
 * ```js
 * a.option({foo: 'a'});
 * b.option({foo: 'b'});
 * c.option({foo: 'c'});
 *
 * app.compose(base, ['a', 'b', 'c'])
 *   .options();
 *
 * console.log(app.options);
 * //=> {foo: 'c'}
 * ```
 * @name .compose.options
 * @param {String} `key` Optionally pass the name of a property to merge from the `options` object. Dot-notation may be used for nested properties.
 * @return {Object} Returns the `Compose` instance for chaining
 * @api public
 */

Compose.prototype.options = function(key) {
  this.iterator(function(generator, app) {
    if (key && typeof key === 'string') {
      app.option(key, generator.option(key));
    } else {
      app.option(generator.options);
    }
  });
  return this;
};

/**
 * Merge the `cache.data` object from each generator onto the `app.cache.data` object.
 * This method requires the `.data()` method from [templates][].
 *
 * ```js
 * a.data({foo: 'a'});
 * b.data({foo: 'b'});
 * c.data({foo: 'c'});
 *
 * app.compose(base, ['a', 'b', 'c'])
 *   .data();
 *
 * console.log(app.cache.data);
 * //=> {foo: 'c'}
 * ```
 * @name .compose.data
 * @param {String} `key` Optionally pass a key to merge from the `data` object.
 * @return {Object} Returns the `Compose` instance for chaining
 * @api public
 */

Compose.prototype.data = function(prop) {
  if (typeof this.receiver.data !== 'function') {
    throw new Error('expected the base-data plugin to be registered');
  }

  this.iterator(function(generator, app) {
    if (prop && typeof prop === 'string') {
      app.data(prop, generator.data(prop));
    } else {
      app.data(generator.cache.data);
    }
  });

  return this;
};

/**
 * Merge the engines from each generator into the `app` engines.
 * This method requires the `.engine()` methods from [templates][].
 *
 * ```js
 * app.compose(base, ['a', 'b', 'c'])
 *   .engines();
 * ```
 * @name .compose.engines
 * @return {Object} Returns the `Compose` instance for chaining
 * @api public
 */

Compose.prototype.engines = function() {
  if (typeof this.receiver.engine !== 'function') {
    throw new Error('.engines requires an instance of templates');
  }

  this.iterator(function(generator, app) {
    utils.merge(app.engines, generator.engines);
  });

  return this;
};

/**
 * Merge the helpers from each generator into `app.helpers`. Requires the
 * `.helper` method from [templates][].
 *
 * ```js
 * app.compose(base, ['a', 'b', 'c'])
 *   .helpers();
 * ```
 * @name .compose.helpers
 * @return {Object} Returns the `Compose` instance for chaining
 * @api public
 */

Compose.prototype.helpers = function() {
  if (typeof this.receiver.helper !== 'function') {
    throw new Error('.helpers requires an instance of templates');
  }

  this.iterator(function(generator, app) {
    app.asyncHelpers(generator._.helpers.async);
    app.helpers(generator._.helpers.sync);
  });

  return this;
};

/**
 * Merge `generator.questions.cache` from specified generators onto `app.questions.cache`.
 * Requires the [base-questions][] plugin to be registered.
 *
 * ```js
 * app.compose(base, ['a', 'b', 'c'])
 *   .questions();
 * ```
 * @name .compose.questions
 * @return {Object} Returns the `Compose` instance for chaining
 * @api public
 */

Compose.prototype.questions = function() {
  if (typeof this.receiver.questions === 'undefined') {
    throw new Error('expected the base-questions plugin to be registered');
  }

  this.iterator(function(generator, app) {
    for (var key in generator.questions.cache) {
      if (generator.questions.cache.hasOwnProperty(key)) {
        app.question(key, generator.questions.cache[key]);
      }
    }
  });

  return this;
};

/**
 * Merge the pipeline plugins from each generator onto `app.plugins`.
 * Requires the [base-pipeline][] plugin to be registered.
 *
 * ```js
 * app.compose(base, ['a', 'b', 'c'])
 *   .pipeline();
 * ```
 * @name .compose.pipeline
 * @return {Object} Returns the `Compose` instance for chaining
 * @api public
 */

Compose.prototype.pipeline = function() {
  if (typeof this.receiver.pipeline !== 'function') {
    throw new Error('expected the base-pipeline plugin to be registered');
  }

  this.iterator(function(generator, app) {
    for (var key in generator.plugins) {
      app.plugin(key, generator.plugins[key]);
    }
  });

  return this;
};

/**
 * Copy the specified tasks and task-dependencies from each generator
 * onto `app.tasks`. Requires using the [base-task][] plugin to be registered.
 *
 * ```js
 * app.compose(base, ['a', 'b', 'c'])
 *   .tasks(['foo', 'bar', 'default']);
 *
 * // or to copy all tasks
 * app.compose(base, ['a', 'b', 'c'])
 *   .tasks();
 * ```
 * @name .compose.tasks
 * @param {String|Array} `tasks` One or more task names (optional)
 * @return {Object} Returns the `Compose` instance for chaining
 * @api public
 */

Compose.prototype.tasks = function(tasks) {
  if (typeof this.receiver.task !== 'function') {
    throw new Error('expected the base-task plugin to be registered');
  }

  this.iterator(function(generator, app) {
    utils.arrayify(tasks || Object.keys(generator.tasks)).forEach(function(task) {
      utils.copyTask(generator, app, task);
    });
  });

  return this;
};

/**
 * Copy view collections and views from each generator onto `app`.
 * Expects `app` to be an instance of [templates][].
 *
 * ```js
 * app.compose(base, ['a', 'b', 'c'])
 *   .views();
 * ```
 * @name .compose.views
 * @param {Array|String} `names` (optional) Names of one or more collections to copy. If undefined all collections will be copied.
 * @param {Function} `filter` Optionally pass a filter function to filter views copied from each collection. The filter function exposes `key`, `view` and `collection` as arguments. If used, the function must return `true` to copy a view.
 * @return {Object} Returns the `Compose` instance for chaining
 * @api public
 */

Compose.prototype.views = function(names, filter) {
  if (typeof this.receiver.create !== 'function') {
    throw new Error('.views requires an instance of templates');
  }

  if (typeof names === 'function') {
    filter = names;
    names = null;
  }

  names = utils.arrayify(names);

  this.iterator(function(generator, app) {
    var len = names.length;
    var idx = -1;

    if (len === 0) {
      names = Object.keys(generator.views);
      len = names.length;
    }

    while (++idx < len) {
      var name = names[idx];
      var collection = generator[name];

      if (typeof app[name] !== 'function' || typeof app[name].views === 'undefined') {
        app.create(collection.options.inflection, collection.options);
      }

      var views = collection.views;

      if (typeof filter === 'function') {
        for (var key in views) {
          if (views.hasOwnProperty(key)) {
            var view = views[key];

            if (filter(key, view, views)) {
              app[name].addView(key, view);
            }
          }
        }
      } else {
        app[name].addViews(views);
      }
    };
  });

  return this;
};

/**
 * Returns an iterator function for iterating over an array of generators.
 * The iterator takes a `fn` that exposes the current generator being iterated
 * over (`generator`) and the app passed into the original function as arguments.
 * No binding is done within the iterator so the function passed in can be
 * safely bound.
 *
 * ```js
 * app.compose(base, ['a', 'b', 'c'])
 *   .iterator(function(generator, app) {
 *     // do work
 *     app.data(generator.cache.data);
 *   });
 *
 * // optionally pass an array of additional generator names as the
 * // first argument. If generator names are defined on `iterator`,
 * // any names passed to `.compose()` will be ignored.
 * app.compose(base, ['a', 'b', 'c'])
 *   .iterator(['d', 'e', 'f'], function(generator, app) {
 *     // do stuff to `generator` and `app`
 *   });
 * ```
 * @name .compose.iterator
 * @param  {Array} `names` Names of generators to iterate over (optional).
 * @param  {Function} `iteratorFn` Function to invoke for each generator in `generators`. Exposes `app` and `generator` as arguments.
 * @return {Object} Returns the `Compose` instance for chaining
 * @api public
 */

Compose.prototype.iterator = function(names, iteratorFn) {
  if (typeof names === 'function') {
    iteratorFn = names;
    names = null;
  }

  names = utils.arrayify(names || this.generators);
  var len = names.length;
  var idx = -1;

  while (++idx < len) {
    var name = names[idx];
    var generator = name;
    try {
      if (typeof name === 'string') {
        generator = this.provider.getGenerator(name);
        if (typeof generator === 'undefined') {
          if (name === 'default') {
            continue;
          }
          throw new Error('generator "' + name + '" is not registered');
        }
      }
      iteratorFn(generator, this.receiver);
    } catch (err) {
      if (this.receiver.hasListeners('error')) {
        this.receiver.emit('error', err);
      } else {
        throw err;
      }
    }
  }
  return this;
};

/**
 * Expose `Compose`
 */

module.exports = Compose;
