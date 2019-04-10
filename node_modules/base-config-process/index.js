'use strict';

var debug = require('debug')('base:config:process');
var fields = require('./lib/fields/');
var utils = require('./lib/utils');

/**
 * Custom mappings for the base-config plugin.
 */

module.exports = function(options) {
  return function plugin(app, base) {
    if (!utils.isValid(app, 'base-config-process')) return;
    debug('initializing <%s>, called from <%s>', __filename, module.parent.id);

    var opts = createOpts(app, options);
    var schema;

    if (typeof this.cwd === 'undefined') {
      this.use(utils.cwd(opts));
    }
    if (typeof this.option === 'undefined') {
      this.use(utils.option(opts));
    }
    if (typeof this.config === 'undefined') {
      this.use(utils.config(opts));
    }

    Object.defineProperty(this.config, 'schema', {
      configurable: true,
      enumerable: true,
      set: function(val) {
        schema = val;
      },
      get: function() {
        return schema || utils.schema(app, createOpts(app, options));
      }
    });

    // add commands
    for (var key in fields) {
      debug('mapping field "%s"', key);
      app.config.map(key, fields[key](app, base, opts));
    }

    var fn = this.config.process;

    this.config.process = function(config, cb) {
      debug('normalizing config object', config);
      var defaults = {
        sortArrays: false,
        omitEmpty: true,
        keys: ['init', 'run', 'toc', 'layout', 'tasks', 'options', 'data', 'plugins', 'related', 'reflinks']
      };

      var obj = this.schema.normalize(config, createOpts(app, options, defaults));

      debug('processing config object', obj);
      fn.call(this, obj, function(err) {
        if (err) {
          cb(err);
          return;
        }
        cb(null, obj);
      });
    };

    return plugin;
  };
};

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
