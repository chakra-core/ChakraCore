/*!
 * to-choices <https://github.com/jonschlinkert/to-choices>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var merge = require('mixin-deep');
var gray = require('ansi-gray');

/**
 * Create a question object with the given `name`
 * and array of `choices`.
 *
 * ```js
 * toChoices(name[, options]);
 * ```
 * @param {String|Object} `name` Name of the question or object with a `name` property.
 * @param {Object|Array} `options` Question options or array of choices. If an array of choices is specified, the name will be used as the `message`.
 * @return {Object}
 * @api public
 */

function toChoices(config) {
  return function fn(name, message, options) {
    if (Array.isArray(options)) {
      return fn(name, message, {choices: options});
    }
    if (Array.isArray(message)) {
      return fn(name, {choices: message}, options);
    }
    if (typeof message === 'string') {
      return fn(name, {message: message}, options);
    }
    if (typeof name === 'string') {
      return fn({name: name}, message, options);
    }

    var question = merge({type: 'checkbox'}, config, options, message, name);
    if (typeof question.message === 'undefined') {
      question.message = question.name;
    }

    /**
     * Generate the list of choices
     */

    var choices = [];
    var len = question.choices.length;
    var idx = -1;

    // create `all` choice
    if (len > 1 && question.all !== false && question.type === 'checkbox') {
      choices.unshift({line: question.separator || gray('·······'), type: 'separator'});
      choices.unshift({name: 'all', value: question.choices.slice()});
    }

    while (++idx < len) {
      var ele = question.choices[idx];
      ele = toItem(ele);
      choices.push(ele);
    }

    question.choices = choices;
    return question;
  };
};

function toItem(ele) {
  if (typeof ele === 'string') {
    ele = { name: ele };
  }
  if (typeof ele.name === 'undefined') {
    if (typeof ele.value !== 'undefined') {
      ele.name = ele.value;
    } else {
      throw new Error('choice items defined as an object must have a `name` property');
    }
  }
  if (typeof ele.value === 'undefined') {
    ele.value = ele.name;
  }
  if (typeof ele.key === 'undefined') {
    ele.key = ele.name[0];
  }
  return ele;
}

/**
 * Expose `toChoices`
 */

module.exports = toChoices;
