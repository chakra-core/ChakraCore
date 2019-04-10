/*!
 * scaffold <https://github.com/jonschlinkert/scaffold>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var Base = require('base');
var util = require('expand-utils');
var utils = require('./utils');

/**
 * Create a new Scaffold with the given `options`
 *
 * ```js
 * var scaffold = new Scaffold({cwd: 'src'});
 * scaffold.addTargets({
 *   site: {src: ['*.hbs']},
 *   blog: {src: ['*.md']}
 * });
 * ```
 * @param {Object} `options`
 * @api public
 */

function Scaffold(options) {
  if (!(this instanceof Scaffold)) {
    return new Scaffold(options);
  }

  Base.call(this);
  this.use(utils.plugins());
  this.is('scaffold');

  options = options || {};
  this.options = options;
  this.targets = {};

  if (Scaffold.isScaffold(options)) {
    this.options = {};
    foward(this, options);
    this.addTargets(options);
    return this;
  } else {
    foward(this, options);
  }
}

/**
 * Inherit `Base`
 */

Base.extend(Scaffold);

/**
 * Static method, returns `true` if the given value is an
 * instance of `Scaffold` or appears to be a valid `scaffold`
 * configuration object.
 *
 * ```js
 * Scaffold.isScaffold({});
 * //=> false
 *
 * var blog = new Scaffold({
 *   post: {
 *     src: 'content/post.md',
 *     dest: 'src/posts/'
 *   }
 * });
 * Scaffold.isScaffold(blog);
 * //=> true
 * ```
 * @static
 * @param {Object} `val` The value to check
 * @return {Boolean}
 * @api public
 */

Scaffold.isScaffold = function(val) {
  return util.isTask(val) || utils.isScaffold(val);
};

/**
 * Add targets to the scaffold, while also normalizing src-dest mappings and
 * expanding glob patterns in each target.
 *
 * ```js
 * scaffold.addTargets({
 *   site: {src: '*.hbs', dest: 'templates/'},
 *   docs: {src: '*.md', dest: 'content/'}
 * });
 * ```
 * @param {Object} `targets` Object of targets, `options`, or arbitrary properties.
 * @return {Object}
 * @api public
 */

Scaffold.prototype.addTargets = function(config) {
  if (!utils.isObject(config)) {
    throw new TypeError('expected an object');
  }

  if (util.isTarget(config)) {
    return this.addTarget(config.name, config);
  }

  for (var key in config) {
    if (config.hasOwnProperty(key)) {
      var val = config[key];

      if (util.isTarget(val)) {
        this.addTarget(key, val);

      } else {
        this[key] = val;
      }
    }
  }
  return this;
};

/**
 * Add a single target to the scaffold, while also normalizing src-dest mappings and
 * expanding glob patterns in the target.
 *
 * ```js
 * scaffold.addTarget('foo', {
 *   src: 'templates/*.hbs',
 *   dest: 'site'
 * });
 *
 * // other configurations are possible
 * scaffold.addTarget('foo', {
 *   options: {cwd: 'templates'}
 *   files: [
 *     {src: '*.hbs', dest: 'site'},
 *     {src: '*.md', dest: 'site'}
 *   ]
 * });
 * ```
 * @param {String} `name`
 * @param {Object} `config`
 * @return {Object}
 * @api public
 */

Scaffold.prototype.addTarget = function(name, config) {
  if (typeof name !== 'string') {
    throw new TypeError('expected name to be a string');
  }

  if (!utils.isObject(config)) {
    throw new TypeError('expected an object');
  }

  var self = this;
  var Target = this.get('Target');
  var target = new Target(this.options);
  utils.define(target, 'parent', this);
  utils.define(target, 'name', name);
  utils.define(target, 'key', name);

  target.on('files', function(stage, files) {
    utils.define(files, 'parent', target);
    self.emit('files', stage, files);
  });

  target.options = utils.extend({}, target.options, config.options);
  this.emit('target', target);
  this.run(target);

  target.addFiles(config);
  this.targets[name] = target;
  return target;
};

/**
 * Getter/setter for the `Target` constructor to use for creating new targets.
 *
 * ```js
 * var Target = scaffold.get('Target');
 * var target = new Target();
 * ```
 * @name .Target
 * @return {Function} Returns the `Target` constructor to use for creating new targets.
 * @api public
 */

Object.defineProperty(Scaffold.prototype, 'Target', {
  configurable: true,
  set: function(Target) {
    utils.define(this, '_Target', Target);
  },
  get: function() {
    return this._Target || this.options.Target || utils.Target;
  }
});

/**
 * Getter/setter for `scaffold.name`. The `name` property can be set on the options or
 * directly on the instance.
 *
 * ```js
 * var scaffold = new Scaffold({name: 'foo'});
 * console.log(scaffold.name);
 * //=> 'foo'
 *
 * // or
 * var scaffold = new Scaffold();
 * scaffold.options.name = 'bar';
 * console.log(scaffold.name);
 * //=> 'bar'
 *
 * // or
 * var scaffold = new Scaffold();
 * scaffold.name = 'baz';
 * console.log(scaffold.name);
 * //=> 'baz'
 * ```
 * @name .name
 * @return {Function} Returns the `Target` constructor to use for creating new targets.
 * @api public
 */

Object.defineProperty(Scaffold.prototype, 'name', {
  configurable: true,
  set: function(val) {
    utils.define(this, '_scaffoldName', val);
  },
  get: function() {
    return this._scaffoldName || this.options.name;
  }
});

/**
 * Forward events from the scaffold instance to the `Scaffold` constructor
 */

function foward(app, options) {
  if (typeof options.name === 'string') {
    app.name = options.name;
    delete options.name;
  }
  Scaffold.emit('scaffold', app);
  emit('target', app, Scaffold);
  emit('files', app, Scaffold);
}

/**
 * Forward events from emitter `a` to emitter `b`
 */

function emit(name, a, b) {
  a.on(name, b.emit.bind(b, name));
}

/**
 * Expose `Scaffold`
 */

module.exports = Scaffold;
