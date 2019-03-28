'use strict';

var hasOwn = Object.prototype.hasOwnProperty;
var utils = require('./utils');

/**
 * Specify a fallback value to use when the desired
 * value is undefined. Note that undefined variables
 * that are _not object properties_ with throw an error.
 *
 * ```js
 * // when `title` is undefined, use the generic `site.title`
 * <%= fallback(page.title, site.title) %>
 * ```
 *
 * @param  {*} `a` The desired value.
 * @param  {*} `b` The fallback ("default") value
 * @return {*} Either `a` or `b`
 * @api public
 */

exports.fallback = function fallback(a, b) {
  return a != null ? a : b;
};

/**
 * Stringify an object using `JSON.stringify()`.
 *
 * ```js
 * <%= stringify({a: "a"}) %>
 * //=> '{"a":"a"}'
 * ```
 *
 * @param  {Object} `object`
 * @return {String}
 * @api public
 */

exports.stringify = function stringify(obj) {
  return JSON.stringify(obj);
};

/**
 * Parse a string into an object using `JSON.parse()`.
 *
 * ```js
 * <%= parse('{"foo":"bar"}')["foo"] %>
 * //=> 'bar'
 * ```
 *
 * @param  {String} `str` The string to parse.
 * @return {Object} The parsed object.
 * @api public
 */

exports.parse = function parse(str) {
  return JSON.parse(str);
};

/**
 * Use property paths (`a.b.c`) get a nested value from an object.
 *
 * ```js
 * <%= get({a: {b: 'c'}}, 'a.b') %>
 * //=> 'c'
 * ```
 *
 * @param  {Object} `object`
 * @param  {String} `path` Dot notation for the property to get.
 * @return {String}
 * @api public
 */

exports.get = function get(obj, prop) {
  return utils.get.apply(utils.get, arguments);
};

/**
 * Returns an array of keys from the given `object`.
 *
 * ```js
 * <%= keys({a: 'b', c: 'd'}) %>
 * //=> '["a", "c"]'
 * ```
 *
 * @param  {Object} `object`
 * @return {Array} Keys from `object`
 * @api public
 */

exports.keys = function keys(obj) {
  return Object.keys(obj);
};

/**
 * Return true if the given `value` is an object, and
 * not `null` or an array.
 *
 * ```js
 * <%= isObject(['a', 'b', 'c']) %>
 * //=> 'false'
 *
 * <%= isObject({a: 'b'}) %>
 * //=> 'true'
 * ```
 *
 * @param  {Object} `value` The value to check.
 * @return {Boolean}
 * @api public
 */

exports.isObject = function isObject(val) {
  return utils.typeOf(val) === 'object';
};

/**
 * Return true if the given `value` is a plain object.
 *
 * ```js
 * <%= isPlainObject(['a', 'b', 'c']) %>
 * //=> 'false'
 *
 * <%= isPlainObject({a: 'b'}) %>
 * //=> 'true'
 *
 * <%= isPlainObject(/foo/g) %>
 * //=> 'false'
 * ```
 *
 * @param  {Object} `value` The value to check.
 * @return {Boolean}
 * @api public
 */

exports.isPlainObject = function isPlainObject(val) {
  return utils.isPlainObject(val);
};

/**
 * Return true if `key` is an own, enumerable property
 * of the given `obj`.
 *
 * @param  {Object} `object`
 * @param  {String} `key`
 * @return {Boolean}
 * @api public
 */

exports.hasOwn = function hasOwn_(obj, key) {
  return hasOwn.call(obj, key);
};

/**
 * Return a copy of `object` exclusing the given `keys`.
 *
 * ```js
 * <%= omit({a: 'a', b: 'b', c: 'c'}, ['a', 'c']) %>
 * //=> '{b: "b"}'
 * ```
 *
 * @param  {Object} `object` Object with keys to omit.
 * @param  {String} `keys` Keys to omit.
 * @return {Boolean}
 * @api public
 */

exports.omit = function omit(o, keys) {
  return utils.omit.apply(utils.omit, arguments);
};

/**
 * Return a copy of `object` exclusing the given `keys`.
 *
 * ```js
 * <%= omit({a: 'a', b: 'b', c: 'c'}, ['a', 'c']) %>
 * //=> '{b: "b"}'
 * ```
 *
 * @param  {Object} `object` Object with keys to omit.
 * @param  {String} `keys` Keys to omit.
 * @return {Boolean}
 * @api public
 */

exports.forIn = function forIn(obj, fn, context) {
  for (var key in obj) {
    if (fn.call(context, obj[key], key, obj) === false) {
      break;
    }
  }
};

/**
 * Return a copy of `object` exclusing the given `keys`.
 *
 * ```js
 * <%= omit({a: 'a', b: 'b', c: 'c'}, ['a', 'c']) %>
 * //=> '{b: "b"}'
 * ```
 *
 * @param  {Object} `object` Object with keys to omit.
 * @param  {String} `keys` Keys to omit.
 * @return {Boolean}
 * @api public
 */

exports.forOwn = function forOwn(obj, fn, context) {
  exports.forIn(obj, function(val, key) {
    if (hasOwn.call(obj, key)) {
      return fn.call(context, obj[key], key, obj);
    }
  });
};

/**
 * Extend `o` with properties of other `objects`.
 *
 * @param  {Object} `o` The target object. Pass an empty object to shallow clone.
 * @param  {Object} `objects`
 * @return {Object}
 * @api public
 */

exports.extend = function extend(obj) {
  if (utils.typeOf(obj) !== 'object') { return ''; }
  var args = arguments;
  var len = args.length - 1;

  if (len === 0) {
    return obj;
  }

  for (var i = 0; i < len; i++) {
    var val = args[i + 1];

    if (utils.typeOf(val) === 'object') {
      for (var key in val) {
        if (exports.hasOwn(val, key)) {
          obj[key] = val[key];
        }
      }
    }
  }
  return obj;
};

/**
 * Recursively combine the properties of `o` with the
 * properties of other `objects`.
 *
 * @param  {Object} `o` The target object. Pass an empty object to shallow clone.
 * @param  {Object} `objects`
 * @return {Object}
 * @api public
 */

exports.merge = function merge(obj) {
  if (utils.typeOf(obj) !== 'object') { return ''; }
  var args = arguments;
  var len = args.length - 1;

  if (len === 0) {
    return obj;
  }

  for (var i = 0; i < len; i++) {
    var val = args[i + 1];

    for (var key in val) {
      if (exports.hasOwn(val, key)) {
        if (utils.typeOf(val[key]) === 'object') {
          obj[key] = exports.merge(obj[key], val[key]);
        } else {
          obj[key] = val[key];
        }
      }
    }
  }
  return obj;
};
