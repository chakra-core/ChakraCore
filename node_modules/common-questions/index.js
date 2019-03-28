/*!
 * common-questions <https://github.com/generate/common-questions>
 *
 * Copyright (c) 2015-2017, Jon Schlinkert.
 * Released under the MIT License.
 */

'use strict';

var debug = require('debug')('common-questions');
var questions = require('./lib/questions');
var listener = require('./lib/listener');
var utils = require('./lib/utils');
var hints = require('./lib/hints');

module.exports = function(config) {
  config = config || {};

  return function(app) {
    if (!utils.isValid(app, 'common-questions')) return;
    debug('initializing from <%s>', __filename);

    var opts = utils.extend({}, app.base.options, app.options, config);
    app.cache.answers = app.cache.answers || {};
    app.cache.data = app.cache.data || {};

    /**
     * Plugins
     */

    app.use(utils.project());
    app.use(utils.pkg());
    app.use(utils.questions(opts));

    /**
     * Default prompt messages.
     *
     * These can be be overridden in your app
     * by creating a new question with the same key. For example, the
     * folling will override the `name` question:
     *
     * ```js
     * app.question('name', 'custom message...');
     * ```
     */

    questions(app, opts);

    /**
     * Hints for common-questions prompts.
     */

    listener(app, opts);
  };
};

module.exports.disableHints = hints;
module.exports.questions = questions;
module.exports.listener = listener;
