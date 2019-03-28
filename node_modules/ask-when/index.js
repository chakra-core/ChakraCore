'use strict';

var debug = require('debug')('ask-when');
var merge = require('mixin-deep');
var isValid = require('is-valid-app');
var isAnswer = require('is-answer');
var get = require('get-value');
var names = [];

function askWhen(app, name, options, cb) {
  debug('initializing from <%s>', __filename);

  if (typeof options === 'function') {
    cb = options;
    options = {};
  }

  if (typeof cb !== 'function') {
    throw new TypeError('expected a callback function');
  }

  var opts = merge({save: false}, app.base.options, app.options, options);
  var skip = get(opts, ['question.skip', name].join('.'));
  var data = merge({}, app.base.cache.data, opts.data, app.base.cache.answers);
  var val = get(data, name) || get(opts, name);

  var answers = {};
  answers[name] = val;

  var isAnswered = isAnswer(val);
  opts.force = isAnswered !== true;

  if (isSkipped(app, names, skip, isAnswered)) {
    opts.askWhen = 'not-answered';
    opts.force = false;
  }

  // conditionally prompt the user
  switch (opts.askWhen) {
    case 'never':
      cb(null, answers);
      return;

    case 'not-answered':
      if (isAnswered) {
        cb(null, answers);
        return;
      }
      break;

    case 'always':
    default: {
      break;
    }
  }
  app.ask(name, opts, cb);
};

module.exports = function(options) {
  return function(app) {
    if (!isValid(app, 'ask-when')) return;

    app.define('askWhen', function() {
      if (typeof this.questions === 'undefined') {
        throw new Error('expected the base-questions plugin to be defined');
      }
      return askWhen.bind(null, this).apply(null, arguments);
    });

    if (app.questions) {
      app.questions.askWhen = app.askWhen.bind(app);
    }
  };
};

function isSkipped(app, names, skip, isAnswered) {
  if (names.indexOf(app.namespace) === -1) {
    names.push(app.namespace);
  }

  if (skip === true && names.length > 1) {
    return true;
  }

  if ((app.options.askWhen === 'not-answered' || names.length > 1) && isAnswered) {
    return true;
  }
  return false;
}

module.exports.when = askWhen;
