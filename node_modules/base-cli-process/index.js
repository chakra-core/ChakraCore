'use strict';

var assert = require('assert');
var debug = require('debug')('base:cli:process');
var fields = require('./lib/fields/');
var utils = require('./lib/utils');

/**
 * Custom mappings for the base-cli plugin.
 */

module.exports = function(config) {
  config = config || {};

  return function(app, base) {
    if (!utils.isValid(app, 'base-cli-process')) return;
    debug('initializing <%s>, called from <%s>', __filename, module.parent.id);

    app.use(assertPlugin);
    var options = createOpts(app, config);
    var schema;

    initPlugins(this, options);

    Object.defineProperty(this.cli, 'schema', {
      cliurable: true,
      enumerable: true,
      set: function(val) {
        schema = val;
      },
      get: function() {
        return schema || utils.schema(app, createOpts(app, config));
      }
    });

    // add commands
    for (var key in fields) {
      debug('mapping field "%s"', key);
      app.cli.map(key, fields[key](app, base, options));
    }

    var fn = this.cli.process;

    this.cli.process = function(val, cb) {
      debug('normalizing argv object', val);

      var defaults = {sortArrays: false, omitEmpty: true};
      var keys = ['get', 'set', 'toc', 'layout', 'options', 'data', 'plugins', 'related', 'reflinks', 'init', 'run', 'tasks'];

      var opts = createOpts(app, config, defaults);
      var obj = sortObject(this.schema.normalize(val, opts), keys);

      debug('processing normalized argv', obj);
      fn.call(this, obj, function(err) {
        if (err) return cb(err);
        cb(null, obj);
      });
    };
  };
};

function initPlugins(app, options) {
  app.use(utils.cwd());

  if (typeof app.option === 'undefined') {
    app.use(utils.option(options));
  }
  if (typeof app.pkg === 'undefined') {
    app.use(utils.pkg(options));
  }
  if (typeof app.config === 'undefined') {
    app.use(utils.config(options));
  }
  if (typeof app.cli === 'undefined') {
    app.use(utils.cli(options));
  }
}

function sortObject(obj, keys) {
  if (Array.isArray(keys) && keys.length) {
    keys = utils.arrUnion(keys, Object.keys(obj));
    var len = keys.length, i = -1;
    var res = {};
    while (++i < len) {
      var key = keys[i];
      if (obj.hasOwnProperty(key)) {
        res[key] = obj[key];
      }
    }
    return res;
  }
}

function createOpts(app, config, defaults) {
  if (typeof defaults !== 'undefined') {
    config = utils.merge({}, defaults, config);
  }
  var options = utils.merge({}, config, app.options);
  if (options.schema) {
    return utils.merge({}, options, options.schema);
  }
  return options;
}

function assertPlugin(app) {
  app.define('assertPlugin', function(name) {
    assert(app.registered.hasOwnProperty(name));
  });
}
