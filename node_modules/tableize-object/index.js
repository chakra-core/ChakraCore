/*!
 * tableize-object (https://github.com/jonschlinkert/tableize-object)
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var isObject = require('isobject');

/**
 * Tableize `obj` by flattening its keys.
 *
 * @param {Object} `obj`
 * @return {Object}
 * @api public
 */

module.exports = function tableize(obj) {
  var target = {};
  flatten(target, obj, '');
  return target;
};

/**
 * Recursively flatten object keys to use dot-notation.
 *
 * @param {Object} `target`
 * @param {Object} `obj`
 * @param {String} `parent`
 */

function flatten(target, obj, parent) {
  for (var key in obj) {
    if (obj.hasOwnProperty(key)) {
      var val = obj[key];

      key = parent + key;
      if (isObject(val)) {
        flatten(target, val, key + '.');
      } else {
        target[key] = val;
      }
    }
  }
}
