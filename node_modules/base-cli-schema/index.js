/*!
 * base-cli-schema <https://github.com/jonschlinkert/base-cli-schema>
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var processArgv = require('./lib/argv');
var debug = require('./lib/debug');
var fields = require('./lib/fields');
var utils = require('./lib/utils');

module.exports = function(app, options) {
  debug.schema('initializing <%s>, called from <%s>', __filename, module.parent.id);

  if (typeof app.cwd !== 'string') {
    throw new Error('expected the base-cwd plugin to be registered');
  }
  if (typeof app.pkg === 'undefined') {
    throw new Error('expected the base-pkg plugin to be registered');
  }
  if (typeof app.argv !== 'function') {
    throw new Error('expected the base-argv plugin to be registered');
  }

  var opts = utils.merge({sortArrays: false, omitEmpty: true}, options);
  var schema = new utils.Schema(opts);
  schema.app = app;

  // Configuration, settings and data
  schema
    .field('config', ['boolean', 'object', 'string'], {
      normalize: fields.config(app, opts)
    })
    .field('data', ['boolean', 'object', 'string'], {
      normalize: fields.data(app, opts)
    })
    .field('disable', 'string', {
      normalize: fields.disable(app, opts)
    })
    .field('enable', 'string', {
      normalize: fields.enable(app, opts)
    })
    .field('options', ['boolean', 'object', 'string'], {
      normalize: fields.options(app, opts)
    })
    .field('option', ['boolean', 'object', 'string'], {
      normalize: fields.option(app, opts)
    });

  // misc
  schema
    .field('asyncHelpers', ['array', 'string'], {
      normalize: fields.asyncHelpers(app, opts)
    })
    .field('helpers', ['array', 'string'], {
      normalize: fields.helpers(app, opts)
    })
    .field('cwd', ['boolean', 'string'], {
      normalize: fields.cwd(app, opts)
    })
    .field('dest', ['boolean', 'string'], {
      normalize: fields.dest(app, opts)
    })
    .field('emit', ['array', 'boolean', 'string'], {
      normalize: fields.emit(app, opts)
    })
    .field('pkg', ['boolean', 'object'], {
      normalize: fields.pkg(app, opts)
    })
    .field('reflinks', ['array', 'object', 'string'], {
      normalize: fields.reflinks(app, opts)
    })
    .field('related', ['array', 'object', 'string'], {
      normalize: fields.related(app, opts)
    })
    .field('save', ['object', 'string'], {
      normalize: fields.save(app, opts)
    })
    .field('tasks', ['array', 'string'], {
      normalize: fields.tasks(app, opts)
    })
    .field('toc', ['object', 'string', 'boolean'], {
      normalize: fields.toc(app, opts)
    })
    .field('use', ['object', 'string', 'boolean'], {
      normalize: fields.use(app, opts)
    });

  // template related
  schema
    .field('layout', ['object', 'string', 'boolean', 'null'], {
      normalize: fields.layout(app, opts)
    });

  var normalizeField = schema.normalizeField;
  schema.normalizeField = function(key, value, config, options) {
    var field = schema.get(key);
    var isUndefined = typeof value === 'undefined'
      && typeof config[key] === 'undefined'
      && typeof field.default === 'undefined';

    if (isUndefined) {
      return;
    }

    debug.field(key, value);
    normalizeField.apply(schema, arguments);

    if (typeof config[key] !== 'undefined') {
      debug.results(key, config[key]);
    }
    return config[key];
  };

  var normalizeSchema = schema.normalize;
  schema.normalize = function(argv) {
    if (argv.isNormalized) {
      return argv;
    }

    var obj = pluralize(processArgv(app, argv));
    var res = normalizeSchema.call(schema, obj, opts);

    for (var key in utils.aliases) {
      if (res.hasOwnProperty(key)) {
        res[utils.aliases[key]] = res[key];
      }
    }
    utils.define(res, 'isNormalized', true);
    return res;
  };
  return schema;
};

/**
 * Ensure certain keys are pluralized (e.g. fix humans)
 */

function pluralize(obj) {
  var props = ['helper', 'asyncHelper', 'plugin', 'engine', 'task'];
  var res = {};

  for (var key in obj) {
    var val = obj[key];
    if (key === 'config' && utils.isObject(val)) {
      val = pluralize(val);
    }
    if (~props.indexOf(key)) {
      key += 's';
    }
    res[key] = val;
  }
  return res;
}
