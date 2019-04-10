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

    this.define('Questions', this.options.questions || utils.Questions);
    app.cache.answers = app.cache.answers || {};

    /**
     * Events
     */

    this.on('ask', function(val, key, question, answers) {
      var data = utils.merge({}, answers, app.store.data, app.cache.answers, app.cache.data);
      if (app.option('common-config') !== false) {
        data = utils.merge({}, app.questions.common.data, data);
      }
      var answer = utils.get(data, key);
      if (answer) {
        utils.set(answers, key, answer);
      }
    });

    this.on('answer', function(val, key, question, answers) {
      app.base.data(key, val);
      app.data(key, val);
    });

    /**
     * Decorate the `questions` instance onto `app` and lazily
     * invoke the `question-store` lib when a questions related
     * method is called.
     */

    utils.sync(this, 'questions', function fn() {
      if (typeof app.store === 'undefined') {
        this.use(utils.store());
      }

      // return cached instance
      if (fn._questions) return fn._questions;

      var cwd = app.cwd || process.cwd();
      var opts = utils.merge({}, app.options, config);
      opts.data = app.cache.data || {};
      opts.cwd = cwd;

      var Questions = app.Questions;
      var questions = new Questions(opts);
      fn._questions = questions;
      questions.cwd = cwd;

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

    this.define('ask', function(names, options, cb) {
      if (typeof names === 'function') {
        cb = names;
        options = {};
        names = this.questions.queue;
      }

      if (typeof options === 'function') {
        cb = options;
        options = {};
      }

      // create a question from `name` if it doesn't already exist
      if (typeof names === 'string' && !this.questions.has(names)) {
        var msg = (names.charAt(0).toUpperCase() + names.slice(1)).replace(/\?$/, '') + '?';
        this.questions.set.call(this.questions, names, options, msg);
      }

      if (utils.isObject(names) && (!names.type && !names.name)) {
        options = names;
        names = this.questions.queue;
      }

      var opts = utils.extend({}, options);
      opts.data = utils.merge({}, app.store.data, app.cache.answers, app.cache.data);

      this.questions.ask.call(this.questions, names, opts, function(err, answers) {
        if (err) return cb(err);

        for (var key in answers) {
          if (answers.hasOwnProperty(key) && answers[key]) {
            app.set(['cache.answers', key], answers[key]);
          }
        }

        cb(null, answers);
      });
    });

    return plugin;
  };
};
