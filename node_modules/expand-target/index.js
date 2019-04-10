'use strict';

var Base = require('base');
var util = require('expand-utils');
var utils = require('./utils');

/**
 * Create a new Target with the given `options`
 *
 * ```js
 * var target = new Target();
 * target.expand({src: '*.js', dest: 'foo/'});
 * ```
 * @param {Object} `options`
 * @api public
 */

function Target(options) {
  if (!(this instanceof Target)) {
    return new Target(options);
  }

  Base.call(this, {}, options);
  this.use(utils.plugins());
  this.is('Target');

  this.options = options || {};
  this.define('Expand', this.options.Expand || utils.Expand);
  this.on('files', Target.emit.bind(Target, 'files'));

  if (Target.isTarget(options)) {
    this.options = {};
    this.addFiles(options);
    return this;
  }
}

/**
 * Inherit `Base`
 */

Base.extend(Target);

/**
 * Expand src-dest mappings and glob patterns in the given `files` configuration.
 *
 * ```js
 * target.addFiles({
 *   'a/': ['*.js'],
 *   'b/': ['*.js'],
 *   'c/': ['*.js']
 * });
 * ```
 * @param {Object} `files`
 * @return {Object} Instance of `Target` for chaining.
 * @api public
 */

Target.prototype.addFiles = function(files) {
  var config = new this.Expand(this.options);
  config.on('files', this.emit.bind(this, 'files'));

  // run plugins on `config`
  util.run(this, 'target', config);
  config.expand(files);

  for (var key in config) {
    if (config.hasOwnProperty(key) && !(key in this)) {
      this[key] = config[key];
    }
  }
  return this;
};

/**
 * Expose `.isTarget`
 */

Target.isTarget = util.isTarget;

/**
 * Expose `Target`
 */

module.exports = Target;
