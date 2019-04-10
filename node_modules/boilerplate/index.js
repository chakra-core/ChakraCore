'use strict';

var Base = require('base');
var util = require('expand-utils');
var utils = require('./utils');

/**
 * Expand a declarative configuration with scaffolds and targets.
 * Create a new Boilerplate with the given `options`
 *
 * ```js
 * var boilerplate = new Boilerplate();
 *
 * // example usage
 * boilerplate.expand({
 *   jshint: {
 *     src: ['*.js', 'lib/*.js']
 *   }
 * });
 * ```
 * @param {Object} `options`
 * @api public
 */

function Boilerplate(options) {
  if (!(this instanceof Boilerplate)) {
    return new Boilerplate(options);
  }

  Base.call(this, {}, options);
  this.is('boilerplate');
  this.use(utils.plugins());

  emit('scaffold', this, Boilerplate);
  emit('target', this, Boilerplate);
  emit('files', this, Boilerplate);
  emit('file', this, Boilerplate);

  utils.define(this, 'count', 0);
  this.scaffolds = {};

  if (util.isConfig(options)) {
    this.options = {};
    this.expand(options);
    return this;
  }
}

/**
 * Inherit Base
 */

Base.extend(Boilerplate);

/**
 * Returns `true` if the given value is an instance of `Boilerplate` or appears to be a
 * valid `boilerplate` configuration object.
 *
 * ```js
 * boilerplate.isBoilerplate({});
 * //=> false
 *
 * var h5bp = new Boilerplate({
 *   options: {cwd: 'vendor/h5bp/dist'},
 *   root: {src: ['{.*,*.*}'], dest: 'src/'},
 *   // ...
 * });
 * boilerplate.isBoilerplate(h5bp);
 * //=> true
 * ```
 * @param {Object} `config` The value to check
 * @return {Boolean}
 * @api public
 */

Boilerplate.prototype.isBoilerplate = function(config) {
  return Boilerplate.isBoilerplate(config);
};

/**
 * Expand and normalize a declarative configuration into scaffolds, targets,
 * and `options`.
 *
 * ```js
 * boilerplate.expand({
 *   options: {},
 *   marketing: {
 *     site: {
 *       mapDest: true,
 *       src: 'templates/*.hbs',
 *       dest: 'site/'
 *     },
 *     docs: {
 *       src: 'content/*.md',
 *       dest: 'site/'
 *     }
 *   },
 *   developer: {
 *     site: {
 *       mapDest: true,
 *       src: 'templates/*.hbs',
 *       dest: 'site/'
 *     },
 *     docs: {
 *       src: 'content/*.md',
 *       dest: 'site/docs/'
 *     }
 *   }
 * });
 * ```
 * @param {Object} `boilerplate` Boilerplate object with scaffolds and/or targets.
 * @return {Object}
 * @api public
 */

Boilerplate.prototype.expand = function(config) {
  // support targets
  if (util.isTarget(config)) {
    var name = config.name || 'target' + (this.count++);
    this.addTarget(name, config);
    return this;
  }

  // support scaffolds
  if (utils.Scaffold.isScaffold(config) && config.name) {
    this.addScaffold(config.name, config);
    return this;
  }

  for (var key in config) {
    if (key === 'name' || key === 'key') {
      continue;
    }
    if (config.hasOwnProperty(key)) {
      var val = config[key];

      if (this.Scaffold.isScaffold(val)) {
        this.addScaffold(key, val);

      } else if (util.isTarget(val)) {
        this.addTarget(key, val);

      } else {
        this[key] = val;
      }
    }
  }
  return this;
};

/**
 * Add a scaffold to the boilerplate, while also normalizing targets with
 * src-dest mappings and expanding glob patterns in each target.
 *
 * ```js
 * boilerplate.addScaffold('assemble', {
 *   site: {src: '*.hbs', dest: 'templates/'},
 *   docs: {src: '*.md', dest: 'content/'}
 * });
 * ```
 * @emits `scaffold`
 * @param {String} `name` the scaffold's name
 * @param {Object} `config` Scaffold configuration object, where each key is a target or `options`.
 * @return {Object}
 * @api public
 */

Boilerplate.prototype.addScaffold = function(name, config) {
  if (typeof name !== 'string') {
    throw new TypeError('expected a string');
  }

  var Scaffold = this.get('Scaffold');
  var scaffold = new Scaffold(this.options);
  utils.define(scaffold, 'name', name);

  scaffold.options = utils.merge({}, this.options, scaffold.options, config.options);
  scaffold.define('parent', this);

  this.emit('scaffold', scaffold);
  emit('target', scaffold, this);
  emit('files', scaffold, this);
  emit('file', scaffold, this);
  this.run(scaffold);

  scaffold.addTargets(config);
  this.scaffolds[name] = scaffold;
  return scaffold;
};

/**
 * Add a target to the boilerplate, while also normalizing src-dest mappings and
 * expanding glob patterns in the target.
 *
 * ```js
 * boilerplate.addTarget({src: '*.hbs', dest: 'templates/'});
 * ```
 * @emits `target`
 * @param {String} `name` The target's name
 * @param {Object} `target` Target configuration object with either a `files` or`src` property, and optionally a `dest` property.
 * @return {Object}
 * @api public
 */

Boilerplate.prototype.addTarget = function(name, config) {
  if (typeof name !== 'string') {
    throw new TypeError('expected a string');
  }
  if (!utils.isObject(config)) {
    throw new TypeError('expected an object');
  }
  if (!this.scaffolds.hasOwnProperty('default')) {
    this.addScaffold('default', {});
  }
  return this.scaffolds.default.addTarget(name, config);
};

/**
 * Get or set the `Target` constructor to use for creating new targets.
 */

Object.defineProperty(Boilerplate.prototype, 'Target', {
  configurable: true,
  set: function(Target) {
    utils.define(this, '_Target', Target);
  },
  get: function() {
    return this._Target || this.options.Target || utils.Target;
  }
});

/**
 * Get or set the `Scaffold` constructor to use for creating new scaffolds.
 */

Object.defineProperty(Boilerplate.prototype, 'Scaffold', {
  configurable: true,
  set: function(Scaffold) {
    utils.define(this, '_Scaffold', Scaffold);
  },
  get: function() {
    return this._Scaffold || this.options.Scaffold || utils.Scaffold;
  }
});

/**
 * Static method, returns `true` if the given value is an
 * instance of `Boilerplate` or appears to be a valid `boilerplate`
 * configuration object.
 *
 * ```js
 * Boilerplate.isBoilerplate({});
 * //=> false
 *
 * var h5bp = new Boilerplate({
 *   options: {cwd: 'vendor/h5bp/dist'},
 *   root: {src: ['{.*,*.*}'], dest: 'src/'},
 *   // ...
 * });
 * Boilerplate.isBoilerplate(h5bp);
 * //=> true
 * ```
 * @static
 * @param {Object} `config` The value to check
 * @return {Boolean}
 * @api public
 */

Boilerplate.isBoilerplate = function(config) {
  if (!utils.isObject(config)) {
    return false;
  }
  if (config.isBoilerplate) {
    return true;
  }
  for (var key in config) {
    if (utils.Scaffold.isScaffold(config[key])) {
      return true;
    }
  }
  return false;
};

/**
 * Forward events
 */

function emit(name, provider, receiver) {
  provider.on(name, receiver.emit.bind(receiver, name));
}

/**
 * Expose `Boilerplate`
 */

module.exports = Boilerplate;
