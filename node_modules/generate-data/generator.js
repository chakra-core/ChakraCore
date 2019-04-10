'use strict';

var utils = require('./utils');

module.exports = function plugin(app, base) {
  if (!utils.isValid(app, 'generate-data')) return;

  /**
   * Clone `cache.data` before it's modified
   */

  if (!app.has('cache.originalData')) {
    app.set('cache.originalData', utils.clone(app.cache.data));
  }

  /**
   * Use `base-data` plugin
   */

  app.use(utils.data());

  /**
   * Update and normalize data
   */

  app.cache.data.project = app.cache.data.project || {};
  app.data({cwd: app.cwd});
  app.data({year: new Date().getFullYear()});
  app.data(app.pkg.data);

  // restore original user-defined data
  app.data(app.cache.originalData);

  /**
   * "Expand" fields using `expand-pkg`
   */

  var config = expander(app);
  app.data(config.expand(app.cache.data));
  if (!app.has('cache.data.runner')) {
    app.data('runner', {name: 'generate', homepage: 'https://github.com/generate/generate'});
  }

  set(app, 'name');
  set(app, 'varname');
  set(app, 'description');
  set(app, 'alias');
  set(app, 'owner');
  return plugin;
};

/**
 * Set data values on the `app.cache.data.project` object, which is
 * merged onto the context at render time and is available in templats
 * as `project`.
 */

function set(app, prop, val) {
  var data = app.cache.data.project;
  if (typeof val !== 'undefined') {
    data[prop] = val;
  } else if (typeof app.cache.data[prop] !== 'undefined') {
    data[prop] = app.cache.data[prop];
  }
}

/**
 * Add custom fields to `expand-package`
 */

function expander(app) {
  var config = new utils.Expand(app.options);

  config.field('username', 'string', {
    normalize: function(val, key, config, schema) {
      if (utils.isString(val)) return val;
      if (typeof val === 'undefined') {
        schema.update('author', config);
        config.author = config.author || {};
      }

      val = config[key] = utils.repo.username(config, schema.options);
      if (utils.isString(val)) {
        config.author.username = val;
        return val;
      }
    }
  });

  config.field('twitter', 'string', {
    normalize: function(val, key, config, schema) {
      if (utils.isString(val)) return val;
      if (typeof val === 'undefined') {
        schema.update('username', config);
      }
      config.author = config.author || {};
      val = schema.options.twitter || config.author.username;
      if (utils.isString(val)) {
        config.author.twitter = val;
        return val;
      }
    }
  });

  config.field('alias', 'string', {
    normalize: function(val, key, config, schema) {
      var name = config.name || '';
      var toAlias = schema.options.toAlias;
      if (typeof toAlias === 'function') {
        val = toAlias.call(config, name);
      } else {
        val = name.slice(name.lastIndexOf('-') + 1);
        val = utils.camelcase(val);
      }
      config[key] = val;
      set(app, key, val);
      return val;
    }
  });

  config.field('varname', 'string', {
    normalize: function(val, key, config, schema) {
      if (utils.isString(val)) {
        set(app, key, val);
        return val;
      }
      var name = config.name;
      if (typeof schema.options.varname === 'function') {
        config[key] = schema.option.varname(name, config);
      } else {
        config[key] = utils.namify(name);
      }
      val = app.cache.data[key] = config[key];
      set(app, key, val);
      return val;
    }
  });

  if (app.options.config && app.options.config.fields) {
    var fields = app.options.config.fields;

    for (var key in fields) {
      if (fields.hasOwnProperty(key)) {
        config[key] = fields[key];
      }
    }
  }
  return config;
}
