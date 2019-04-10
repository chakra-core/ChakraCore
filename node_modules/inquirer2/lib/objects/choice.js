'use strict';

var utils = require('../utils/');

/**
 * Choice object
 * Normalize input as choice object
 * @param {String|Object} `val` Choice value. If an object is passed, it should contain at least one of `value` or `name` property.
 * @api public
 */

var Choice = module.exports = function(val, answers) {
  // Don't process Choice and Separator object
  if (val instanceof Choice || val.type === 'separator') {
    return val;
  }

  if (typeof val === 'string') {
    this.name = val;
    this.value = val;
    this.short = val;
  } else {
    utils.extend(this, val, {
      name: val.name || val.value,
      value: val.hasOwnProperty('value') ? val.value : val.name,
      short: val.short || val.name || val.value
    });
  }

  if (typeof val.disabled === 'function') {
    this.disabled = val.disabled(answers);
  } else {
    this.disabled = val.disabled;
  }
};
