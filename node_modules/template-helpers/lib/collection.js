'use strict';

var utils = require('./utils');

/**
 * Returns `true` if `value` exists in the given string, array
 * or object. See [any] for documentation.
 *
 * @param {*} `value`
 * @param {*} `target`
 * @param {Object} `options`
 * @api public
 */

exports.any = function any(value, target) {
  return utils.any.apply(utils.any, arguments);
};

/**
 * Filter the given array or object to contain only the matching values.
 *
 * ```js
 * <%= filter(['foo', 'bar', 'baz']) %>
 * //=> '["a", "b", "c"]'
 * ```
 *
 * @param {Array} `arr`
 * @return {Array}
 * @api public
 */

exports.filter = function filter(val, fn, context) {
  if (utils.isEmpty(val)) return '';
  var iterator = function() {};
  if (typeof fn === 'string') {
    var prop = fn;
    iterator = function(target) {
      if (typeof target === 'string') {
        return target === prop;
      }
      return target[prop];
    };
  } else {
    iterator = utils.makeIterator(fn, context);
  }
  if (typeof val === 'string') {
    return iterator(val);
  }
  if (Array.isArray(val)) {
    return val.filter(iterator);
  }

  if (utils.isObject(val)) {
    var obj = val;
    var res = {};
    for (var key in obj) {
      if (obj.hasOwnProperty(key) && iterator(key)) {
        res[key] = obj[key];
      }
    }
    return res;
  }
};
