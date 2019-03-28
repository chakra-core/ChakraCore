/*!
 * map-schema <https://github.com/jonschlinkert/map-schema>
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var util = require('util');
var Emitter = require('component-emitter');
var debug = require('debug')('map-schema');
var Data = require('./lib/data');
var Field = require('./lib/field');
var utils = require('./lib/utils');

/**
 * Expose `Schema`
 */

module.exports = Schema;

/**
 * Create a new `Schema` with the given `options`.
 *
 * ```js
 * var schema = new Schema()
 *   .field('name', 'string')
 *   .field('version', 'string')
 *   .field('license', 'string')
 *   .field('licenses', 'array', {
 *     normalize: function(val, key, config) {
 *        // convert license array to `license` string
 *        config.license = val[0].type;
 *        delete config[key];
 *     }
 *   })
 *   .normalize(require('./package'))
 * ```
 * @param {Object} `options`
 * @api public
 */

function Schema(options) {
  this.options = options || {};
  this.data = new Data();
  this.isSchema = true;
  this.utils = utils;
  this.initSchema();
  this.addFields(this.options);
  var only = utils.arrayify(this.options.pick || this.options.only);
  utils.define(this.options, 'only', only);
}

/**
 * Inherit Emitter
 */

Emitter(Schema.prototype);

/**
 * Initalize Schema instance properties
 */

Schema.prototype.initSchema = function() {
  this.normalizers = {};
  this.validators = {};
  this.defaults = {};
  this.required = [];
  this.warnings = [];
  this.fields = {};
  this.remove = [];
  this.fns = [];
};

/**
 * Set `key` on the instance with the given `value`.
 *
 * @param {String} `key`
 * @param {Object} `value`
 * @api public
 */

Schema.prototype.set = function(key, value) {
  utils.set(this, key, value);
  return this;
};

/**
 * Push a warning onto the `schema.warnings` array. Placeholder for
 * better message handling and a reporter (planned).
 *
 * @param {String} `method` The name of the method where the warning is recorded.
 * @param {String} `prop` The name of the field for which the warning is being created.
 * @param {String} `message` The warning message.
 * @param {String} `value` The value associated with the warning.
 * @return {any}
 * @api public
 */

Schema.prototype.warning = function(method, prop, msg, value) {
  var warning = { method: method, prop: prop, message: msg, args: value };
  if (typeof value !== 'undefined') {
    warning.value = value;
  }
  this.warnings.push(warning);
  this.emit('warning', method, prop, warning);
  return this;
};

/**
 * Add a field to the schema with the given `name`, `type` or types,
 * and options.
 *
 * ```js
 * var semver = require('semver');
 *
 * schema
 *   .field('keywords', 'array')
 *   .field('version', 'string', {
 *     validate: function(val, key, config, schema) {
 *       return semver.valid(val) !== null;
 *     }
 *   })
 * ```
 * @param {String} `name`
 * @param {String|Array} `type`
 * @param {Object} `options`
 * @return {Object} Returns the instance for chaining.
 * @api public
 */

Schema.prototype.field = function(name, type, options) {
  var pick = utils.arrayify(this.options.pick || this.options.only);
  if (pick && pick.length && !utils.hasElement(name, pick)) {
    return this;
  }

  if (typeof options === 'function') {
    options = { normalize: options };
  }

  debug('adding field "%s"', name);
  var field = new Field(type, options || {});
  field.name = name;

  if (field.hasOwnProperty('default')) {
    this.defaults[name] = field.default;
  }
  if (field.required) {
    this.required.push(name);
  }

  this.fields[name] = field;
  return this;
};

/**
 * Add an object of fields to `schema.fields`. If `options.extend` is true,
 * and a field with the given name already exists, the new field will extend
 * the existing field.
 *
 * @param {Object} `options`
 */

Schema.prototype.addFields = function(options) {
  options = options || {};
  if (options.fields) {
    for (var key in options.fields) {
      var val = options.fields[key];
      if (this.fields[key] && (val.extend || options.extend)) {
        val = utils.merge({}, val, this.fields[key]);
      }
      this.field(key, val);
    }
  }
  return this;
};

/**
 * Get field `name` from the schema. Get a specific property from
 * the field by passing the property name as a second argument.
 *
 * ```js
 * schema.field('bugs', ['object', 'string']);
 * var field = schema.get('bugs', 'types');
 * //=> ['object', 'string']
 * ```
 * @param {Strign} `name`
 * @param {String} `prop`
 * @return {Object|any} Returns the field instance or the value of `prop` if specified.
 * @api public
 */

Schema.prototype.get = function(name, prop) {
  return utils.get(this.fields, prop ? [name, prop] : name);
};

/**
 * Omit a property from the returned object. This method can be used
 * in normalize functions as a way of removing undesired properties.
 *
 * @param {String} `key` The property to remove
 * @return {Object} Returns the instance for chaining.
 * @api public
 */

Schema.prototype.omit = function(key) {
  this.remove.push(key);
  return this;
};

/**
 * Remove `key` from the `config` object.
 *
 * @param {String} key
 * @param {Object} config
 * @return {undefined} The object is modified in place
 */

Schema.prototype.removeKey = function(key, config) {
  if (this.remove.indexOf(key) !== -1) {
    delete this.defaults[key];
    delete config[key];
    return true;
  }
};

/**
 * Update a property on the returned object. This method will trigger validation
 * and normalization of the updated property.
 *
 * @param {String} `key` The property to update.
 * @param {*} `val` Value of the property to update.
 * @return {Object} Returns the instance for chaining.
 * @api public
 */

Schema.prototype.update = function(key, val, config) {
  debug('updating field "%s"', key);

  if (arguments.length === 2) {
    config = val;
    val = config[key];
  }

  if (typeof val !== 'undefined' && config) {
    config[key] = val;
  }

  this.normalizeField(key, val, config);
  return this;
};

/**
 * Returns true if field `name` is an optional field.
 *
 * @param {String} `name`
 * @return {Boolean}
 * @api public
 */

Schema.prototype.isOptional = function(name) {
  return this.get(name, 'optional') === true;
};

/**
 * Returns true if field `name` was defined as a required field.
 *
 * @param {String} `name`
 * @return {Boolean}
 * @api public
 */

Schema.prototype.isRequired = function(name) {
  return this.get(name, 'required') === true;
};

/**
 * When `options.defaults` is true, any default values defined
 * on fields will be set on properties that aren't set or do not
 * have a value already define.
 *
 * We need to loop over the config again, since defaults might
 * be defined for properties that don't exist on the config.
 *
 * @param {Object} `config`
 * @return {Object} returns the config object with defaults defined.
 */

Schema.prototype.setDefaults = function(config) {
  config = utils.extend({}, config);
  if (this.options.defaults === false) {
    return config;
  }
  for (var key in this.defaults) {
    if (!utils.hasValue(key, config)) {
      config[key] = this.defaults[key];
    }
  }
  return config;
};

/**
 * Checks the config object for missing fields and. If found,
 * a warning message is pushed onto the `schema.warnings` array,
 * which can be used for reporting.
 *
 * @param {Object} `config`
 * @return {Array}
 * @api public
 */

Schema.prototype.missingFields = function(config) {
  if (this.options.required === false) {
    return false;
  }
  var len = this.required.length, i = -1;
  var res = [];
  while (++i < len) {
    var prop = this.required[i];
    if (!config.hasOwnProperty(prop)) {
      this.warning('missing', prop);
      res.push(prop);
    }
  }
  return res;
};

/**
 * If a `keys` array is passed on the constructor options, or
 * as a second argument to `sortObject`, this sorts the given
 * object so that keys are in the same order as the supplied
 * array of `keys`.
 *
 * ```js
 * schema.sortObject({z: '', a: ''}, ['a', 'z']);
 * //=> {a: '', z: ''}
 * ```
 * @param {Object} `config`
 * @return {Object} Returns the config object with keys sorted to match the given array of keys.
 * @api public
 */

Schema.prototype.sortObject = function(config, options) {
  var opts = utils.merge({}, this.options, options);
  var keys = opts.keys;

  if (Array.isArray(keys) && keys.length) {
    keys = utils.union(keys, Object.keys(config));
    var len = keys.length, i = -1;
    var res = {};
    while (++i < len) {
      var key = keys[i];
      if (config.hasOwnProperty(key)) {
        res[key] = config[key];
      }
    }
    return res;
  }
  return config;
};

/**
 * When `options.sortArrays` _is not false_, sorts all arrays in the
 * given `config` object using JavaScript's native `.localeCompare`
 * method.
 *
 * @param {Object} `config`
 * @return {Object} returns the config object with sorted arrays
 * @api public
 */

Schema.prototype.sortArrays = function(config) {
  if (this.options.sortArrays !== false) {
    return utils.sortArrays(config);
  }
  return config;
};

/**
 * Returns true if the given value is valid for field `key`.
 *
 * @param {String} `key`
 * @param {any} `val`
 * @param {Object} `config`
 * @return {Boolean}
 * @api public
 */

Schema.prototype.isValidField = function(key) {
  var field = this.get(key);
  if (typeof field === 'undefined') {
    return this.options.knownOnly !== true;
  }
  return field.isValidType(this.config[key], key, this.config, this);
};

Schema.prototype.isValidType = function(key, val, config) {
  config = config || {};
  this.config = config;

  var field = this.get(key);
  if (typeof field === 'undefined') {
    if (this.options.knownOnly) {
      this.warning('invalidField', key);
      return false;
    }
    return true;
  }

  if (field.isValidType(val, key, config, this)) {
    return true;
  }

  if (config.hasOwnProperty(key) || typeof field.normalize === 'function') {
    if (this.isRequired(key) || typeof val !== 'undefined') {
      var types = this.get(key, 'types');
      var warning = {expected: types, actual: val};
      this.warning('invalidType', key, warning);
    }
  }
  return false;
};

/**
 * Iterate over the given `config` and validate any properties
 * that have a `field` with a `validate` function on the schema.
 *
 * @param {Object} `config`
 * @return {Object}
 */

Schema.prototype.validate = function(config) {
  if (!utils.isObject(config)) {
    throw new TypeError('expected config to be an object');
  }
  for (var key in config) {
    this.validateField(config[key], key, config);
  }
  return this.warnings;
};

/**
 * Validate a single property on the config.
 *
 * @param {any} `val`
 * @param {String} `key`
 * @param {Object} `config`
 * @return {undefined}
 */

Schema.prototype.validateField = function(val, key, config) {
  var field = this.fields[key];

  if (!utils.isObject(field) && this.options.knownOnly === true) {
    this.warning('invalidField', key);
  }

  this.isValidType(key, val, config);

  if (typeof field.validate === 'function') {
    var isValid = field.validate(val, key, config, this);
    if (!isValid) {
      this.warning('invalidValue', key, {actual: val});
    }
    return isValid;
  }
};

/**
 * Normalize the given `config` object.
 *
 * @param {String} key
 * @param {any} value
 * @param {Object} config
 * @return {Object}
 * @api public
 */

Schema.prototype.normalize = function(config, options) {
  debug('normalizing config');

  if (utils.typeOf(config) !== 'object') {
    throw new TypeError('expected config to be an object');
  }

  options = options || {};
  this.addFields(options);
  this.config = config;

  // set defaults and call normalizers
  config = this.setDefaults(config);
  var opts = utils.merge({}, this.options, options);

  var pick = utils.arrayify(opts.only || opts.pick);
  if (pick.length) {
    this.fields = utils.pick(this.fields, pick);
  }

  for (var key in this.fields) {
    if (opts.existingOnly === true && !config.hasOwnProperty(key)) {
      continue;
    }

    if (this.fields.hasOwnProperty(key)) {
      this.normalizeField.call(this, key, config[key], config, opts);
    }
  }

  // check for missing required fields
  this.missingFields(config);

  // get omitted keys
  var omitted = utils.arrayify(opts.omit);

  // remove empty objects if specified on options
  if (opts.omitEmpty === true) {
    config = utils.omitEmpty(config);
  }

  if (pick.length) {
    config = utils.pick(config, pick);
  }

  var omit = utils.union([], this.remove, omitted);
  config = utils.omit(config, omit);

  // sort object and arrays
  config = this.sortObject(config, opts);
  config = this.sortArrays(config);

  this.logWarnings(config);
  utils.define(config, 'isNormalized', true);
  this.emit('normalized', config);
  return config;
};

/**
 * Normalize a field on the schema.
 *
 * @param {String} key
 * @param {any} value
 * @param {Object} config
 * @return {Object}
 * @api public
 */

Schema.prototype.normalizeField = function(key, value, config, options) {
  debug('normalizing field "%s", "%j"', key, util.inspect(value));

  if (!this.fields.hasOwnProperty(key)) {
    this.removeKey(key, config);
    return;
  }

  var field = this.fields[key];
  var val = config[key];

  if (utils.isObject(field)) {
    field.isNormalized = true;
    var fn = field.normalize;

    if (typeof fn === 'function' && !this.options.validate) {
      if (field.isSchema && field.warnings.length) {
        utils.union(this.warnings, field.warnings);
      }

      val = fn.call(this, val, key, config, this);
      if (field.isValidType(val)) {
        config[key] = val;
      }

      if (this.removeKey(key, config)) {
        return;
      }
    }

    if (this.isValidType(key, val, config) === false) {
      return;
    }

    if (typeof field.validate === 'function' && !this.options.normalize) {
      if (!this.validateField(val, key, config)) {
        return;
      }
    }
  } else {
    this.isValidType(key, val, config);
  }

  this.removeKey(key, config);
};

/**
 * Visit `method` over the given object or array.
 *
 * @param {String} `method`
 * @param {Object|Array} `value`
 * @return {Object} Returns the instance for chaining.
 * @api public
 */

Schema.prototype.visit = function(method, value) {
  utils.visit(this, method, value);
  return this;
};

/**
 * Log warnings and warnings that were recorded during normalization.
 *
 * This is a placeholder for a reporter (planned)
 *
 * @return {String}
 */

Schema.prototype.logWarnings = function(warnings) {
  warnings = warnings || this.warnings;
  var omit = utils.arrayify(this.options.omit);
  var len = warnings.length;
  var idx = -1;

  if (this.options.verbose !== true || len === 0) {
    return;
  }

  var max = utils.longest(Object.keys(this.fields)).length;
  var msg = '\n';

  // warnings body
  while (++idx < len) {
    if (idx > 0) {
      msg += '\n';
    }
    var warning = warnings[idx];
    if (omit.indexOf(warning.prop) !== -1) {
      continue;
    }
    msg += utils.formatWarning(warning, max);
  }

  // log out warnings
  console.log(msg);
};
