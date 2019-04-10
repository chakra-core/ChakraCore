/*!
 * base-questions <https://github.com/jonschlinkert/base-questions>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var debug = require('debug')('base-questions');
var utils = require('./utils');

module.exports = function(config, fn) {
  return function plugin(app) {
    if (!utils.isValid(app, 'base-questions')) return;
    debug('initializing from <%s>', __filename);

    /**
     * Decorate the `questions` instance onto `app` and lazily
     * invoke the `question-store` lib when a questions related
     * method is called.
     */

    utils.sync(this, 'questions', function fn() {
      if (typeof app.store === 'undefined') {
        this.use(utils.store());
      }

      var opts = utils.merge({}, app.options, config);

      // return cached instance
      if (fn._questions) return fn._questions;

      opts.store = app.store;
      opts.globals = app.globals;
      opts.data = app.cache.data || {};
      opts.cwd = app.cwd || process.cwd();

      var Questions = utils.Questions;
      var questions = new Questions(opts);
      fn._questions = questions;

      var globals = questions.globals;
      var store = questions.store;
      var data = questions.data;

      questions.on('ask', app.emit.bind(app, 'ask'));
      questions.on('answer', app.emit.bind(app, 'answer'));
      questions.on('error', function(err) {
        err.reason = 'base-questions error';
        app.emit('error', err);
      });

      Object.defineProperty(questions, 'data', {
        set: function(val) {
          data = val;
        },
        get: function() {
          data = utils.merge({}, data, opts.data);
          return utils.clone(data);
        }
      });

      utils.sync(questions, 'store', function() {
        return app.store || store;
      });

      utils.sync(questions, 'globals', function() {
        return app.globals || globals;
      });

      return questions;
    });

    /**
     * Create a `confirm` question.
     *
     * ```js
     * app.confirm('file', 'Want to generate a file?');
     *
     * // equivalent to
     * app.question({
     *   name: 'file',
     *   message: 'Want to generate a file?',
     *   type: 'confirm'
     * });
     * ```
     * @name .confirm
     * @param {String} `name` Question name
     * @param {String} `msg` Question message
     * @param {String|Array} `queue` Name or array of question names.
     * @param {Object|Function} `options` Question options or callback function
     * @param {Function} `callback` callback function
     * @api public
     */

    this.define('confirm', function() {
      this.questions.confirm.apply(this.questions, arguments);
      return this.questions;
    });

    /**
     * Create a "choices" question from an array.
     *
     * ```js
     * app.choices('color', 'Favorite color?', ['blue', 'orange', 'green']);
     *
     * // or
     * app.choices('color', {
     *   message: 'Favorite color?',
     *   choices: ['blue', 'orange', 'green']
     * });
     *
     * // or
     * app.choices({
     *   name: 'color',
     *   message: 'Favorite color?',
     *   choices: ['blue', 'orange', 'green']
     * });
     * ```
     * @name .choices
     * @param {String} `name` Question name
     * @param {String} `msg` Question message
     * @param {Array} `choices` Choice items
     * @param {String|Array} `queue` Name or array of question names.
     * @param {Object|Function} `options` Question options or callback function
     * @param {Function} `callback` callback function
     * @api public
     */

    this.define('choices', function() {
      this.questions.choices.apply(this.questions, arguments);
      return this.questions;
    });

    /**
     * Add a question to be asked by the `.ask` method.
     *
     * ```js
     * app.question('beverage', 'What is your favorite beverage?');
     *
     * // or
     * app.question('beverage', {
     *   type: 'input',
     *   message: 'What is your favorite beverage?'
     * });
     *
     * // or
     * app.question({
     *   name: 'beverage'
     *   type: 'input',
     *   message: 'What is your favorite beverage?'
     * });
     * ```
     * @name .question
     * @param {String} `name` Question name
     * @param {String} `msg` Question message
     * @param {Object|String} `value` Question object, message (string), or options object.
     * @param {String} `locale` Optionally pass the locale to use, otherwise the default locale is used.
     * @return {Object} Returns the `this.questions` object, for chaining
     * @api public
     */

    this.define('question', function() {
      return this.questions.set.apply(this.questions, arguments);
    });

    /**
     * Ask one or more questions, with the given `options` and callback.
     *
     * ```js
     * // ask all questions
     * app.ask(function(err, answers) {
     *   console.log(answers);
     * });
     *
     * // ask the specified questions
     * app.ask(['name', 'description'], function(err, answers) {
     *   console.log(answers);
     * });
     * ```
     * @name .ask
     * @param {String|Array} `queue` Name or array of question names.
     * @param {Object|Function} `options` Question options or callback function
     * @param {Function} `callback` callback function
     * @api public
     */

    this.define('ask', function(queue, options, cb) {
      if (typeof queue === 'function') {
        return this.ask(this.questions.queue, {}, queue);
      }
      if (options === 'function') {
        return this.ask(queue, {}, options);
      }
      if (utils.isObject(queue)) {
        return this.ask(this.questions.queue, queue, options);
      }
      if (typeof queue === 'string' && !this.questions.has(queue)) {
        this.questions.set.call(this.questions, queue, options, queue);
      }
      this.questions.ask.call(this.questions, queue, options, cb);
    });

    return plugin;
  };
};
