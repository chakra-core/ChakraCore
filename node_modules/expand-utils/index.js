/*!
 * expand-utils <https://github.com/jonschlinkert/expand-utils>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var utils = require('./utils');

/**
 * Decorates the given object with a `is*` method, where `*`
 * is the given `name`.
 *
 * ```js
 * var obj = {};
 * exports.is(obj, 'foo');
 * console.log(obj.isFoo);
 * //=> true
 * ```
 * @param {Object} `obj` The object to check
 * @param {String} `name`
 * @api public
 */

exports.is = function(obj, name) {
  var key = name.charAt(0).toUpperCase() + name.slice(1);
  utils.define(obj, 'is' + key, true);
  utils.define(obj, 'key', name);
};

/**
 * Run `parent`'s plugins on the `child` object, specifying a `key`
 * to use for decorating the `child` object with an [is](#is) property.
 *
 * ```js
 * exports.run(parent, 'foo', obj);
 * ```
 * @api public
 */

exports.run = function(parent, key, child) {
  if (!parent.hasOwnProperty('run') || typeof parent.run !== 'function') {
    var val = JSON.stringify(parent);
    throw new TypeError('expected `' + val + '` to have a "run" method');
  }

  utils.define(child, 'parent', parent);
  exports.is(child, key);
  parent.run(child);
  delete child[key];
};

/**
 * Return true if the given value has [boilerplate][] properties
 *
 * ```js
 * exports.isBoilerplates({});
 * ```
 * @param {Object} `config` The configuration object to check
 * @api public
 */

exports.isBoilerplate = function(config) {
  if (!utils.isObject(config)) {
    return false;
  }
  if (config.isBoilerplate) {
    return true;
  }
  for (var key in config) {
    if (exports.isScaffold(config[key])) {
      return true;
    }
  }
  return false;
};

/**
 * Return true if the given value has [config](https://github.com/jonschlinkert/expand-config) properties
 *
 * ```js
 * exports.isConfig({});
 * ```
 * @param {Object} `config` The configuration object to check
 * @api public
 */

exports.isConfig = function(config) {
  if (!utils.isObject(config)) {
    return false;
  }
  if (exports.isTask(config)) {
    return true;
  }
  if (exports.isTarget(config)) {
    return true;
  }
  for (var key in config) {
    if (exports.isTask(config[key])) {
      return true;
    }
  }
  return false;
};

/**
 * Return true if the given value has [scaffold][] properties. Scaffolds are essentially the
 * same configuration structure as tasks from [expand-task][]. We use [expand-task][] for expanding
 * [grunt][]-style configs. But we use [scaffold][] when working with [gulp][] configs.
 *
 * ```js
 * console.log(exports.isScaffold({
 *   foo: {
 *     options: {},
 *     files: []
 *   },
 *   bar: {
 *     options: {},
 *     files: []
 *   }
 * }));
 * //=> true
 * ```
 * @param {Object} `config` The configuration object to check
 * @api public
 */

exports.isScaffold = function(config) {
  if (!utils.isObject(config)) {
    return false;
  }
  if (config.isScaffold === true) {
    return true;
  }
  return exports.isTask(config);
};

/**
 * Return true if the given value has [task](https://github.com/jonschlinkert/expand-task) properties
 *
 * ```js
 * console.log(exports.isTask({
 *   foo: {
 *     options: {},
 *     files: []
 *   },
 *   bar: {
 *     options: {},
 *     files: []
 *   }
 * }));
 * //=> true
 * ```
 * @param {Object} `config` The configuration object to check
 * @api public
 */

exports.isTask = function(config) {
  if (!utils.isObject(config)) {
    return false;
  }
  if (config.isTask === true) {
    return true;
  }
  if (exports.isFiles(config)) {
    return false;
  }
  if (exports.isTarget(config)) {
    return false;
  }
  for (var key in config) {
    if (exports.isTarget(config[key])) {
      return true;
    }
  }
  return false;
};

/**
 * Return true if the given value is has [target](https://github.com/jonschlinkert/expand-target)
 * properties.
 *
 * ```js
 * console.log(exports.isTarget({
 *   name: 'foo',
 *   options: {},
 *   files: []
 * }));
 * //=> true
 * ```
 * @param {Object} `config` The configuration object to check
 * @api public
 */

exports.isTarget = function(config) {
  if (!utils.isObject(config)) {
    return false;
  }
  if (config.isTarget === true) {
    return true;
  }
  if (exports.hasFilesProps(config)) {
    return true;
  }
  return false;
};

/**
 * Return true if the given value is an object and instance of [expand-files](https://github.com/jonschlinkert/expand-files), has an `isFiles` property, or returns true from [hasFilesProps]().
 *
 * ```js
 * console.log(exports.hasFilesProps({isFiles: true}));
 * //=> true
 * console.log(exports.hasFilesProps({files: []}));
 * //=> true
 * console.log(exports.hasFilesProps({files: [{src: [], dest: ''}]}));
 * //=> true
 * console.log(exports.hasFilesProps({src: []}));
 * //=> true
 * console.log(exports.hasFilesProps({dest: ''}));
 * //=> true
 * console.log(exports.hasFilesProps({foo: ''}));
 * //=> false
 * ```
 * @param {Object} `config` The configuration object to check
 * @api public
 */

exports.isFiles = function(config) {
  if (!utils.isObject(config)) {
    return false;
  }
  if (config.isFiles === true) {
    return true;
  }
  if (exports.hasFilesProps(config)) {
    return true;
  }
  return false;
};

/**
 * Return true if the given value is an object that has
 * `src`, `dest` or `files` properties.
 *
 * ```js
 * console.log(exports.hasFilesProps({files: []}));
 * //=> true
 * console.log(exports.hasFilesProps({files: [{src: [], dest: ''}]}));
 * //=> true
 * console.log(exports.hasFilesProps({src: []}));
 * //=> true
 * console.log(exports.hasFilesProps({dest: ''}));
 * //=> true
 * console.log(exports.hasFilesProps({foo: ''}));
 * //=> false
 * ```
 * @param {Object} `config` The configuration object to check
 * @api public
 */

exports.hasFilesProps = function(config) {
  if (!utils.isObject(config)) {
    return false;
  }
  if (config.hasOwnProperty('files')) {
    return true;
  }
  if (config.hasOwnProperty('src')) {
    return true;
  }
  if (config.hasOwnProperty('dest')) {
    return true;
  }
  return false;
};
