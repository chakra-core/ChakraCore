'use strict';

var os = require('os');
var util = require('util');
var path = require('path');
var debug = require('debug')('question-store');
var Cache = require('question-cache');
var utils = require('./lib/utils');

/**
 * Answer stores, for persisting answers across sessions
 */

var globals;
var store;
var hints;

/**
 * Create an instance of `QuestionsStore` with the given `options`.
 *
 * ```js
 * var QuestionsStore = new QuestionsStore(options);
 * ```
 * @param {Object} `options` question store options
 * @api public
 */

function QuestionsStore(options) {
  debug('initializing from <%s>', __filename);
  Cache.call(this, options);
  this.createStores(this, this.options);
  this.listen(this);
}

/**
 * Inherit `quesstion-cache`
 */

util.inherits(QuestionsStore, Cache);

/**
 * Create stores for persisting data across sessions.
 *
 * - `globals`: Persist non-project-specific answers when `question.options.global` is true
 * - `store`: Persist project-specific answers
 * - `hints`: Persist project-specific hints. This is used to populate the `question.default` value.
 *
 * @param {Object} `options`
 * @return {Object}
 * @api public
 */

QuestionsStore.prototype.createStores = function(app, options) {
  // persist answers to questions with `{ global: true }`
  utils.sync(this, 'globals', function() {
    debug('creating globals store');
    if (typeof globals === 'undefined') {
      debug('created globals store');
      globals = options.globals || new utils.Store(path.join('question-store', 'globals'), {
        cwd: os.homedir()
      });
    }
    return globals;
  });

  // load common-config
  utils.sync(this, 'common', function() {
    return utils.common;
  });

  // persist project-specific answers
  utils.sync(this, 'store', function() {
    debug('creating project store');
    if (typeof store === 'undefined') {
      store = options.store || new utils.Store(path.join('question-store', app.project || ''));
      debug('created project store');
    }
    return store;
  });

  // persist cwd-specific hints
  utils.sync(this, 'hints', function() {
    debug('creating hints store');
    if (typeof hints === 'undefined') {
      hints = options.hints || new utils.Store(path.join('question-store', utils.slugify(process.cwd()), 'hints'));
      debug('created hints store');
    }
    return hints;
  });
};

QuestionsStore.prototype.listen = function(app) {
  this.on('ask', function(val, key, question, answers) {
    if (!utils.isAnswer(val) && app.enabled('common')) {
      val = question.answer = app.common.get(key);
      debug('no answer found, using common-config: "%s"', val);
    }
    if (!utils.isAnswer(val) && app.enabled('global')) {
      question.answer = app.globals.get(key);
      debug('no answer found, using global-store: "%s"', val);
    }
  });

  this.on('answer', function(val, key, question, answers) {
    var options = question._options;

    // persist to 'project' store if 'save' is not disabled
    if (options.enabled('save')) {
      debug('saving answer in project store: %j', val);
      app.store.set(key, val);
    }

    // persist to 'global-globals' store if 'global' is enabled
    if (options.enabled('global')) {
      debug('saving answer in global store: %j', val);
      app.globals.set(key, val);
    }

    // persist to project-specific 'hint' store, if 'hint' is not disabled
    if (!options.disabled('hint')) {
      debug('saving answer in hints store: %j', val);
      app.hints.set(key, val);
    }
  });
};

/**
 * Expose `QuestionsStore`
 */

module.exports = QuestionsStore;
