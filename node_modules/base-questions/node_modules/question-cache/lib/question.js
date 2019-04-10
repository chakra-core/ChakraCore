/*!
 * question-store <https://github.com/jonschlinkert/question-store>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var util = require('util');
var debug = require('debug')('questions:question');
var utils = require('./utils');

/**
 * Create new `Question` store `name`, with the given `options`.
 *
 * ```js
 * var question = new Question(name, options);
 * ```
 * @param {String} `name` The question property name.
 * @param {Object} `options` Store options
 * @api public
 */

function Question(name, message, options) {
  if (utils.isObject(name)) {
    options = utils.merge({}, message, name);
    message = options.message;
    name = options.name;
  }

  if (utils.isObject(message)) {
    options = utils.merge({}, options, message);
    message = options.message;
  }

  utils.define(this, 'isQuestion', true);
  utils.define(this, 'cache', {});

  this.options = options || {};
  this.type = this.options.type || 'input';
  this.message = message || this.options.message;
  this.name = name || this.options.name;

  if (!this.message) {
    this.message = this.name;
  }

  utils.merge(this, this.options);
  createNext(this);
}

/**
 * Create the next question to ask, if `next` is passed on the
 * options.
 *
 * @param {Object} `app` question instance
 */

function createNext(question) {
  if (!question.options.next) return;

  if (typeof question.options.next === 'function') {
    question.next = function() {
      question.options.next.apply(question, arguments);
    };
    return;
  }

  if (typeof question.options.next === 'string') {
    question.type = 'confirm';
    question.next = function(answer, questions, answers, next) {
      if (answer === true) {
        questions.ask(question.options.next, next);
      } else {
        next(null, answers);
      }
    };
  }
}

/**
 * Optionally define the next question to ask by setting a custom `next`
 * function on the question object.
 *
 * ```js
 * questions.choice('deps', 'Where?')
 * questions.set('install', 'Want to install?', {
 *   type: 'confirm',
 *   next: function(answer, questions, answers, cb) {
 *     if (answer === true) {
 *       questions.ask('config.other', cb);
 *     } else {
 *       cb(null, answers);
 *     }
 *   }
 * });
 * ```
 *
 * @param {Object} `answer`
 * @param {Object} `questions`
 * @param {Object} `answers`
 * @param {Function} `next`
 * @return {Function}
 * @api public
 */

Question.prototype.next = function(answer, questions, answers, next) {
  next(null, answers);
};

/**
 * Merge the given `options` object with `questions.options` and expose
 * `get`, `set` and `enabled` properties to simplify checking for options values.
 *
 * @param {Object} options
 * @return {Object}
 */

Question.prototype.opts = function(options) {
  var args = [].slice.call(arguments);
  options = [].concat.apply([], args);

  var opts = utils.omitEmpty(utils.merge.apply(utils.merge, [{}].concat(options)));
  utils.decorate(opts);

  if (typeof opts.default !== 'undefined') {
    this.default = opts.default;
  }
  if (opts.persist === false) {
    opts.global = false;
    opts.hint = false;
    opts.save = false;
  }
  if (opts.save === false) {
    opts.force = true;
  }
  return opts;
};

Question.prototype.setAnswer = function(val) {
  this.answer = val;
  return this;
};

/**
 * Resolve the answer for the question from the given data sources, then
 * set the question's `default` value with any stored hints or answers
 * if not already defined.
 *
 * ```js
 * question.answer(answers, data, store, hints);
 * ```
 * @param {Object} `answers`
 * @param {Object} `data`
 * @param {Object} `store`
 * @param {Object} `hints`
 * @return {Object}
 * @api public
 */

Question.prototype.getAnswer = function(answers, data) {
  return utils.get(data, this.name) || utils.get(answers, this.name);
};

/**
 * Force the question to be asked.
 *
 * ```js
 * question.force();
 * ```
 * @return {undefined}
 * @api public
 */

Question.prototype.force = function() {
  this.options.force = true;
  return this;
};

/**
 * Expose `Question`
 */

module.exports = Question;
