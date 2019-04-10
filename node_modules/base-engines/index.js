'use strict';

var debug = require('debug')('base:engines');
var utils = require('./utils');

module.exports = function(options) {
  return function baseEngines(app) {
    if (!utils.isValid(app, 'base-engines', ['app', 'collection', 'views', 'list'])) {
      return;
    }

    if (typeof this._ === 'undefined') {
      utils.define(this, '_', {});
    }

    this.engines = this.engines || {};
    this._.engines = new utils.Engines(this.engines);
    this._.helpers = {
      async: {},
      sync: {}
    };

    /**
     * Register a view engine callback `fn` as `ext`. Calls `.setEngine`
     * and `.getEngine` internally.
     *
     * ```js
     * app.engine('hbs', require('engine-handlebars'));
     *
     * // using consolidate.js
     * var engine = require('consolidate');
     * app.engine('jade', engine.jade);
     * app.engine('swig', engine.swig);
     *
     * // get a registered engine
     * var swig = app.engine('swig');
     * ```
     * @name .engine
     * @param {String|Array} `exts` String or array of file extensions.
     * @param {Function|Object} `fn` or `settings`
     * @param {Object} `settings` Optionally pass engine options as the last argument.
     * @api public
     */

    this.define('engine', function(exts, fn, settings) {
      if (arguments.length === 1 && typeof exts === 'string') {
        return this.getEngine(exts);
      }
      if (!Array.isArray(exts) && typeof exts !== 'string') {
        throw new TypeError('expected engine ext to be a string or array.');
      }
      if (settings === 'function') {
        return this.engine(exts, settings, fn);
      }
      utils.arrayify(exts).forEach(function(ext) {
        this.setEngine(ext, fn, settings);
      }.bind(this));
      return this;
    });

    /**
     * Register engine `ext` with the given render `fn` and/or `settings`.
     *
     * ```js
     * app.setEngine('hbs', require('engine-handlebars'), {
     *   delims: ['<%', '%>']
     * });
     * ```
     * @name .setEngine
     * @param {String} `ext` The engine to set.
     * @api public
     */

    this.define('setEngine', function(ext, fn, settings) {
      debug('registering engine "%s"', ext);
      settings = settings || {};
      ext = utils.formatExt(ext);
      if (settings.default === true) {
        this._.engines.defaultEngine = ext;
      }
      this._.engines.setEngine(ext, fn, settings);
      return this;
    });

    /**
     * Get registered engine `ext`.
     *
     * ```js
     * app.engine('hbs', require('engine-handlebars'));
     * var engine = app.getEngine('hbs');
     * ```
     * @name .getEngine
     * @param {String} `ext` The engine to get.
     * @api public
     */

    this.define('getEngine', function(ext, fallback) {
      debug('getting engine "%s"', ext);

      if (!utils.isString(ext)) {
        ext = this.options['view engine']
          || this.options.viewEngine
          || this.options.engine;
      }

      if (utils.isString(ext)) {
        ext = utils.formatExt(ext);
        var engine = this._.engines.getEngine(ext);
        if (!engine && this.options.engine && fallback !== false) {
          return this.getEngine(this.options.engine, false);
        }
        return engine;
      }
    });

    return baseEngines;
  };
};
