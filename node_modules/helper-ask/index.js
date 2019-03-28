'use strict';

var debug = require('debug')('helper:ask');
var isObject = require('isobject');
var merge = require('mixin-deep');
var has = require('has-value');
var get = require('get-value');
var ask = require('ask-when');

/**
 * Async helper that prompts the user and uses the answer as
 * context for rendering templates.
 *
 * ```html
 * <%%= ask('author.name') %>
 * ```
 * @param {Object} `app`
 * @return {Function} Returns the helper function
 * @api public
 */

module.exports = function(app) {
  debug('initializing from <%s>', __filename);
  app.base.cache.answers = app.base.cache.answers || {};
  var cached = {};

  return function(name, message, options, cb) {
    debug('asking <%s>', name);

    var args = [].slice.call(arguments, 1);
    cb = args.pop();

    if (typeof cb !== 'function') {
      throw new TypeError('expected a callback function');
    }

    var answer = getAnswer(app, cached, name);
    if (typeof answer !== 'undefined') {
      cb(null, answer);
      return;
    }

    app = app || this.app;
    var last = args[args.length - 1];
    var opts = {};

    if (isObject(last)) {
      options = args.pop();
    }

    last = args[args.length - 1];
    if (typeof last === 'string') {
      opts.message = args.pop();
    }

    options = merge({}, this.options, options, opts);
    options.data = merge({}, this.context, options.data);
    options.save = false;

    ask.when(this.app, name, options, function(err, answers) {
      if (err) {
        cb(err);
        return;
      }

      var answer = get(answers, name);
      cacheAnswer(app, cached, name, answer);
      app.data(answers);
      cb(null, answer);
    });
  };
};

function getAnswer(app, cached, key) {
  var data = merge({}, app.cache.data, app.base.cache.answers, cached);
  if (!data.homepage && data.repo) {
    data.homepage = 'https://github.com/' + data.repo;
  }

  app.base.data(data);
  var answer;

  if (has(data, key)) {
    answer = get(data, key);
  } else if (key === 'name') {
    answer = app.project;
  } else if (/^author\./.test(key) && app.common && typeof app.common.get === 'function') {
    answer = app.common.get(key);
  }

  if (answer) {
    cacheAnswer(app, cached, key, answer);
  } else if (!has(cached, key)) {
    // ensure unanswered questions still get asked
    answer = undefined;
  }

  return answer;
}

function cacheAnswer(app, cached, key, answer) {
  app.base.cache.answers[key] = answer;
  cached[key] = answer;
  app.base.data(key, answer);
}
