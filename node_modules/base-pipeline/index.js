'use strict';

var path = require('path');
var utils = require('./utils');

module.exports = function(options) {
  return function fn(app) {
    plugin(options).call(this, app);
    if (this.isApp) {
      return fn;
    }
  };
};

function plugin(options) {
  return function(app) {
    if (!utils.isValid(app, 'base-pipeline', ['app', 'collection'])) return;

    this.use(utils.option());
    this.plugins = this.plugins || {};

    /**
     * Register a plugin by `name`
     *
     * @param  {String} `name`
     * @param  {Function} `fn`
     * @api public
     */

    this.define('plugin', function(name, options, fn) {
      if (typeof name !== 'string') {
        throw new TypeError('expected plugin name to be a string');
      }

      var args = [].slice.call(arguments, 1);
      fn = args.pop();
      options = args.length ? args.pop() : {};

      if (typeof fn !== 'function' && !isStream(fn)) {
        options = fn;
        return normalize(this, name, options);
      }

      if (typeof options === 'function' || isStream(options)) {
        fn = options;
        options = {};
      }

      this.option(['plugin', name], options || {});
      this.plugins[name] = fn;
      return this;
    });

    /**
     * Create a plugin pipeline from an array of plugins.
     *
     * @param  {Array} `plugins` Each plugin is a function that returns a stream,
     *                       or the name of a registered plugin.
     * @param  {Object} `options`
     * @return {Stream}
     * @api public
     */

    this.define('pipeline', function(plugins, options) {
      if (isStream(plugins)) return plugins;

      if (isPlugins(plugins)) {
        plugins = [].concat.apply([], [].slice.call(arguments));
        if (utils.typeOf(plugins[plugins.length - 1]) === 'object') {
          options = plugins.pop();
        } else {
          options = {};
        }
      }

      if (utils.typeOf(plugins) === 'object') {
        options = plugins;
        plugins = null;
      }

      if (!plugins) {
        plugins = Object.keys(this.plugins);
      }

      var len = plugins.length;
      var idx = -1;
      var res = [];

      var pass = utils.through.obj();
      pass.resume();
      res.push(pass);

      while (++idx < len) {
        var plugin = normalize(this, plugins[idx], options);
        if (!plugin) continue;
        res.push(plugin);
      }

      var stream = utils.combine(res);
      stream.on('error', this.emit.bind(this, 'error'));
      return stream;
    });
  };
}

function normalize(app, val, options) {
  if (typeof val === 'string' && app.plugins.hasOwnProperty(val)) {
    if (isDisabled(app, 'plugin', val)) return null;
    var name = path.basename(val, path.extname(val));
    var opts = utils.extend({}, app.option(['plugin', name]), options);
    return normalize(app, app.plugins[val], opts);
  }
  if (typeof val === 'function') {
    return val.call(app, options, utils.through.obj());
  }
  if (isStream(val)) {
    return val;
  }
  return null;
}

/**
 * Returns true if the plugin is disabled.
 */

function isDisabled(app, key, prop) {
  // key + '.plugin'
  if (app.isFalse([key, prop])) {
    return true;
  }
  // key + '.plugin.disable'
  if (app.isTrue([key, prop, 'disable'])) {
    return true;
  }
  return false;
};

function isStream(val) {
  return val && typeof val === 'object'
    && typeof val.pipe === 'function';
}

function isPlugins(val) {
  return Array.isArray(val)
    || typeof val === 'function'
    || typeof val === 'string';
}
