/*!
 * base-npm-prompt (https://github.com/node-base/base-npm-prompt)
 *
 * Copyright (c) 2016, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var utils = require('./utils');

module.exports = function(config) {
  return function(app) {
    if (!utils.isValid(app, 'base-npm-prompt')) {
      return;
    }

    if (typeof app.npm === 'undefined') {
      throw new Error('expected the base-npm plugin to be registered');
    }

    var npm = app.npm;

    /**
     * Prompts the user to ask if they want to install the packages listed on
     * `app.cache.install.dependencies` or `app.cache.install.devDependencies` based on
     * the given `type`.
     *
     * ```js
     * app.npm.prompt('dependencies', function(err) {
     *   if (err) return console.error(err):
     * });
     * ```
     * @name .npm.prompt
     * @param {String} `type` dependency type to install (dependencies or devDependencies)
     * @param {Object} `options`
     * @param {Function} `cb` Callback
     * @api public
     */

    utils.define(npm, 'prompt', function(type, options, cb) {
      if (typeof options === 'function') {
        cb = options;
        options = {};
      }

      var opts = utils.extend({type: type}, options);
      install(opts, cb);
    });

    /**
     * Prompts the user to ask if they want to install the given package(s).
     * Requires the [base-questions][] plugin to be registered first.
     *
     * ```js
     * app.npm.askInstall('isobject', function(err) {
     *   if (err) throw err;
     * });
     * ```
     * @name .npm.askInstall
     * @param {String|Array} `names` One or more package names.
     * @param {Object} `options`
     * @param {Function} `cb` Callback
     * @api public
     */

    utils.define(npm, 'askInstall', function(names, options, cb) {
      install(names, options, cb);
    });

    /**
     * Prompts the user to ask if they want to check if the given package(s)
     * exist on npm, then install the ones that do exist.
     * Requires the [base-questions][] plugin to be registered first.
     *
     * ```js
     * app.npm.checkInstall('isobject', function(err) {
     *   if (err) throw err;
     * });
     * ```
     * @name .npm.checkInstall
     * @param {String|Array} `names` One or more package names.
     * @param {Object} `options`
     * @param {Function} `cb` Callback
     * @api public
     */

    utils.define(npm, 'checkInstall', function(names, options, cb) {
      if (typeof names !== 'string' && !Array.isArray(names)) {
        npm.checkInstall([], names, options);
        return;
      }

      if (typeof options === 'function') {
        cb = options;
        options = {};
      }

      // setup options specific for `checkInstall` to
      // show a different message and check the modules exist before installing.
      var opts = utils.extend({
        namespace: 'base-npm-check-install-',
        message: function(names, type) {
          var target = names.length > 1 ? type : `"${names[0]}" to "${type}"`;
          return `Would you like to check for and install ${target}?`;
        },
        handleAnswer: function(type, answer, cb) {
          npm.exists(answer, function(err, exists) {
            if (err) return cb(err);

            utils.each(Object.keys(exists), function(key, next) {
              if (exists[key]) {
                console.log(utils.log.green(utils.log.success), key);
              } else {
                console.log(utils.log.red(utils.log.error), key);
              }
              if (exists[key] === false) return next();
              npm[type](key, next);
            }, cb);
          });
        }
      }, options);

      // run the actual ask and install method
      install(names, opts, cb);
    });

    /**
     * Does the actual ask and installing based on the options passed in.
     * Default logic is the original `askInstall` logic.
     *
     * @param  {String|Array} `names` One or more package names.
     * @param  {Object} `options`
     * @param  {Function} `cb` Callback
     */

    function install(names, options, cb) {
      if (typeof names !== 'string' && !Array.isArray(names)) {
        install([], names, options);
        return;
      }

      if (typeof options === 'function') {
        cb = options;
        options = {};
      }

      // register the base questions plugin
      if (typeof app.ask !== 'function') {
        app.use(utils.questions());
      }

      // extend `data` onto options, which is used
      // by question-store to pre-populate answers
      options = utils.extend({}, options, options.data);

      var type = options.type || 'devDependencies';
      var key = (options.namespace || 'base-npm-install-') + type; // namespace to validate conflicts

      names = arrayify(names);
      names = filterKeys(app, type, names);
      if (names.length === 0) return cb();

      // allow a custom message string or function.
      var msg = options.message || function(names, type) {
        var target = names.length > 1 ? type : `"${names[0]}" to "${type}"`;
        return `Would you like to install ${target}?`;
      };

      // allow a custom answer handler
      var handleAnswer = options.handleAnswer || function(type, answer, cb) {
        npm[type](answer, cb);
      };

      app.on('ask', function(answer, key, question, answers) {
        if (typeof answer !== 'undefined' && question.name === key) {
          question.options.skip = true;
          answers[key] = answer;
        }
      });

      // allow prompt to be suppressed for unit tests
      if (options.noprompt === true) {
        options[key] = true;
      }

      if (typeof msg === 'function') {
        msg = msg(names, type);
      }

      if (names.length === 1) {
        app.confirm(key, {message: msg, save: false});
      } else {
        app.choices(key, {message: msg, save: false}, names);
      }

      // ask the actual question
      app.ask(key, options, function(err, answers) {
        if (err) return cb(err);
        var answer = answers[key];
        if (answer === true) {
          answer = names;
        }

        answer = arrayify(answer);
        if (answer.length) {
          handleAnswer(type, answer, cb);
        } else {
          cb();
        }
      });
    }
  };
};

/**
 * Filter out any package names that have already been installed.
 * Also include any that are stored on `app.cache.install[prop]`.
 */

function filterKeys(app, prop, names) {
  var all = names.concat(app.get(['cache.install', prop]) || []);
  if (!app.pkg || !app.pkg.data) {
    return all;
  }

  var depKeys = Object.keys(app.pkg.get(prop) || {});
  return all.filter(function(name) {
    return depKeys.indexOf(name) === -1;
  });
}

/**
 * Cast `val` to an array
 */

function arrayify(val) {
  return val ? (Array.isArray(val) ? val : [val]) : [];
}
