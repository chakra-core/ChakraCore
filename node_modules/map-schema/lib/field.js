'use strict';

var utils = require('./utils');

/**
 * Expose `Field`
 */

module.exports = Field;

/**
 * Create a new `Field` of the given `type` to validate against,
 * and optional `config` object.
 *
 * ```js
 * var field = new Field('string', {
 *   normalize: function(val) {
 *     // do stuff to `val`
 *     return val;
 *   }
 * });
 * ```
 * @param {String|Array} `type` One more JavaScript native types to use for validation.
 * @param {Object} `config`
 * @api public
 */

function Field(type, config) {
  if (utils.typeOf(type) === 'object') {
    config = type;
    type = null;
  }

  if (!utils.isObject(config)) {
    throw new TypeError('expected config to be an object');
  }

  this.types = type || config.type || config.types || [];
  this.types = typeof this.types === 'string'
    ? this.types.split(/\W/)
    : this.types;

  if (typeof this.types === 'undefined' || this.types.length === 0) {
    throw new TypeError('expected type to be a string or array of JavaScript native types');
  }

  for (var key in config) {
    this[key] = config[key];
  }

  if (!config.hasOwnProperty('required')) {
    this.required = false;
  }
  if (!config.hasOwnProperty('optional')) {
    this.optional = true;
  }
  if (this.required === true) {
    this.optional = false;
  }
  if (this.optional === false) {
    this.required = true;
  }
}

/**
 * Returns true if the given `type` is a valid type.
 *
 * @param {String} `type`
 * @return {Boolean}
 * @api public
 */

Field.prototype.isValidType = function(val) {
  return this.types.indexOf(utils.typeOf(val)) !== -1;
};

/**
 * Called in `schema.validate`, returns true if the given
 * `value` is valid. This default validate method returns
 * true unless overridden with a custom `validate` method.
 *
 * ```js
 * var field = new Field({
 *   types: ['string']
 * });
 *
 * field.validate('name', {});
 * //=> false
 * ```
 *
 * @return {Boolean}
 * @api public
 */

Field.prototype.validate = function(/*val, key, config, schema*/) {
  return true;
};

/**
 * Normalize the field's value.
 *
 * ```js
 * var field = new Field({
 *   types: ['string'],
 *   normalize: function(val, key, config, schema) {
 *     // do stuff to `val`
 *     return val;
 *   }
 * });
 * ```
 * @api public
 */

Field.prototype.normalize = function(val, key, config) {
  config[key] = val;
  return val;
};
