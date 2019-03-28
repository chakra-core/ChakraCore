'use strict';

var path = require('path');
var debug = require('debug')('expand:pkg');
var Emitter = require('component-emitter');
var schema = require('./lib/schema');
var utils = require('./lib/utils');

/**
 * Create an instance of `Config` with the given `options`.
 *
 * ```js
 * var config = new Config();
 * var pkg = config.expand({
 *   author: 'Jon Schlinkert (https://github.com/jonschlinkert)'
 * });
 * console.log(pkg);
 * //=> {name: 'Jon Schlinkert', url: 'https://github.com/jonschlinkert'}
 * ```
 * @param {Object} `options`
 * @api public
 */

function Config(options) {
  if (!(this instanceof Config)) {
    return new Config(options);
  }

  debug('initializing <%s>', __filename);
  this.options = options || {};
  this.schema = schema(this.options);
  this.data = this.schema.data;
  this.schema.on('warning', this.emit.bind(this, 'warning'));
  this.schema.on('error', this.emit.bind(this, 'error'));
}

/**
 * Inherit `Emitter`
 */

Emitter(Config.prototype);

/**
 * Add a field to the schema, or overwrite or extend an existing field. The last
 * argument is an `options` object that supports the following properties:
 *
 * - `normalize` **{Function}**: function to be called on the given package.json value when the `.expand` method is called
 * - `default` **{any}**: default value to be used when the package.json property is undefined.
 * - `required` **{Boolean}**: define `true` if the property is required
 *
 * ```js
 * var config = new Config();
 *
 * config.field('foo', 'string', {
 *   default: 'bar'
 * });
 *
 * var pkg = config.expand({});
 * console.log(pkg);
 * //=> {foo:  'bar'}
 * ```
 *
 * @param {String} `name` Field name (required)
 * @param {String|Array} `type` One or more native javascript types allowed for the property value (required)
 * @param {Object} `options`
 * @return {Object} Returns the instance
 * @api public
 */

Config.prototype.field = function(field, type, options) {
  debug('adding "%s"', field);
  if (typeof options === 'function') {
    options = { normalize: options };
  }
  if (options.extend === true) {
    options = utils.merge({}, this.schema.get(field), options);
  }
  this.schema.field(field, type, options);
  return this;
};

/**
 * Iterate over `pkg` properties and expand values that have corresponding
 * [fields](#field) registered on the schema.
 *
 * ```js
 * var config = new Config();
 * var pkg = config.expand(require('./package.json'));
 * ```
 * @param {Object} `pkg` The `package.json` object to expand
 * @param {Object} `options`
 * @return {Object} Returns an expanded package.json object.
 * @api public
 */

Config.prototype.expand = function(pkg, options) {
  if (typeof pkg === 'undefined') {
    pkg = path.resolve(process.cwd(), 'package.json');
  }
  if (typeof pkg === 'string') {
    pkg = utils.loadPkg.sync(pkg);
  }
  return this.schema.normalize(pkg, options);
};

/**
 * Config
 */

module.exports = Config;
