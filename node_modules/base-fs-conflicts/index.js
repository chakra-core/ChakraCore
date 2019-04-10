'use strict';

var path = require('path');
var Emitter = require('component-emitter');
var utils = require('./lib/utils');
var diff = require('./lib/diffs');

/**
 * Detect potential conflicts between existing files and the path and
 * contents of a vinyl file. If the destination `file.path` already
 * exists on the file system:
 *
 *   1. The existing file's contents is compared with `file.contents` on the vinyl file
 *   2. If the contents of both are identical, no action is taken
 *   3. If the contents differ, the user is prompted for action
 *   4. If no conflicting file exists, the vinyl file is written to the file system
 *
 * ```js
 * app.src('foo/*.js')
 *   .pipe(app.conflicts('foo'))
 *   .pipe(app.dest('foo'));
 * ```
 * @param {String} `dest` The same desination directory passed to `app.dest()`
 * @return {String}
 * @api public
 */

module.exports = function(config) {
  return function(app) {
    var actions = {};

    this.define('conflicts', function(dest, options) {
      if (typeof dest !== 'string') {
        throw new TypeError('expected dest to be a string');
      }

      if (!('cwd' in app)) {
        app.use(utils.cwd());
      }

      var opts = utils.extend({}, config, this.options, options);
      var emitter = Emitter({});
      var files = [];

      emitter.detectConflicts = detectConflicts(emitter, files, actions, opts);

      return utils.through.obj(function(file, enc, next) {
        if (file.isNull() || (file.stat && file.stat.isDirectory())) {
          next();
          return;
        }

        file.dest = typeof dest === 'function'
          ? dest(file)
          : dest;

        emitter.detectConflicts(file, next);
      }, function(next) {
        var self = this;
        files.forEach(function(file) {
          self.push(file);
        });
        next();
      });
    });
  };
};

/**
 * Detect file conflicts and the user how to proceed.
 *
 * The following actions are supported:
 *
 *   - `Y` yes, overwrite this file (this is the default)
 *   - `n` no do not overwrite this file
 *   - `a` overwrite this file and all remaining files
 *   - `x` abort
 *   - `d` show the differences between the old and the new
 *   - `h` help. displays this help menu in the terminal
 *
 * @param {Object} `app` Base application instance
 * @param {Array} `files`
 * @param {Object} `actions`
 * @param {Object} `options`
 */

function detectConflicts(emitter, files, actions, options) {
  var opts = utils.extend({}, options);
  actionsListeners(emitter, files, actions);
  var questions = utils.inquirer();

  return function(file, next) {
    // overwrite the current file
    if (opts.overwrite === true) {
      next(null, file);
      return;
    }
    if (typeof opts.overwrite === 'function' && opts.overwrite(file) === true) {
      next(null, file);
      return;
    }
    // abort
    if (actions.abort) {
      files = [];
      next();
      return;
    }
    // overwrite all files
    if (actions.all === true) {
      files.push(file);
      next();
      return;
    }

    var fp = path.resolve(file.dest, file.relative);
    var conflict = utils.detect(fp, file.contents.toString());
    if (conflict) {
      questions.prompt(createQuestion(file), function(answers) {
        emitter.emit('action', answers.actions, file, next);
      });
    } else {
      files.push(file);
      next();
    }
  };
}

function actionsListeners(emitter, files, actions) {
  emitter.on('action', function(type, file, next) {
    utils.action[type](file);
    emitter.emit('action.' + type, file, next);
  });

  emitter.on('action.yes', function(file, next) {
    files.push(file);
    next();
  });

  emitter.on('action.no', function(file, next) {
    next();
  });

  emitter.on('action.all', function(file, next) {
    actions.all = true;
    files.push(file);
    next();
  });

  emitter.on('action.abort', function(file, next) {
    actions.abort = true;
    next();
  });

  emitter.on('action.diff', function(file, next) {
    diff(file, function(err) {
      if (err) {
        next(err);
        return;
      }
      emitter.detectConflicts(file, next);
    });
  });
}

/**
 * Create the question to ask when a conflict is detected
 *
 * @param {Object} `file` vinyl file
 * @return {Array} Question formatted the way inquirer expects it.
 */

function createQuestion(file) {
  return [{
    name: 'actions',
    type: 'expand',
    save: false,
    message: 'File exists, want to overwrite ' + utils.relative(file) + '?',
    choices: [{
      key: 'y',
      name: 'yes, overwrite this file',
      value: 'yes'
    }, {
      key: 'n',
      name: 'no, do not overwrite this file',
      value: 'no'
    }, {
      key: 'a',
      name: 'overwrite this file and all remaining files',
      value: 'all'
    }, {
      key: 'x',
      name: 'abort',
      value: 'abort'
    }, {
      key: 'd',
      name: 'show the differences between the existing and the new',
      value: 'diff'
    }]
  }];
}
