'use strict';

var fs = require('fs');
var path = require('path');
var plugins = require('./plugins');
var utils = require('./utils');

/**
 * Static methods
 */

module.exports = function(Generate) {

  /**
   * Initialize middleware
   */

  Generate.initGenerateMiddleware = function(app) {
    app.preWrite(/./, function(view, next) {
      var askName = view.data && view.data.ask;
      var hint = view.basename;
      if (utils.isObject(askName)) {
        var obj = askName;
        hint = obj.default || hint;
        askName = obj.rename;
      }

      function setValue(obj) {
        var key = askName;
        var val = obj[key];
        if (val) view[key] = val;
        if (key === 'path') {
          view.base = path.dirname(view.path);
          app.options.dest = view.base;
        }
      }

      if (typeof askName === 'string') {
        if (utils.argv[askName]) {
          setValue(utils.argv);
          next();
          return;
        }

        app.question(askName, `What is the file.${askName}?`, {default: hint});
        app.askWhen(askName, {save: false}, function(err, answers) {
          if (err) return next(err);
          if (answers[askName]) {
            setValue(answers);
          }
          next();
        });
      } else {
        next();
      }
    });

    /**
     * If the user has defined a template in `~/templates/`,
     * use the contents from that file to overwrite the
     * generator's template.
     */

    app.onLoad(/(^|[\\\/])templates[\\\/]/, function(file, next) {
      var userDefined = app.home('templates', file.relative);
      if (fs.existsSync(userDefined)) {
        file.contents = fs.readFileSync(userDefined);
      } else {
        userDefined = app.home('templates', file.basename)
        if (fs.existsSync(userDefined)) {
          file.contents = fs.readFileSync(userDefined);
        }
      }

      // flatten dest file.path by stripping `templates/`
      if (/^templates[\\\/]/.test(file.relative)) {
        file.path = path.join(app.cwd, file.basename);
      }
      next(null, file);
    });
  };

  /**
   * Initialize plugins for CLI
   */

  Generate.initGenerateCLI = function(app, options) {
    var opts = {globals: this.globals, store: this.store};
    plugins.runner.loadPlugins(app);
    app.use(plugins.logger());
    app.use(plugins.questions(opts));
    app.use(plugins.rename({replace: true}));
    app.use(plugins.conflicts(options));
    app.use(plugins.runtimes(options));
    app.use(plugins.loader());
    app.use(plugins.npm());
    app.use(plugins.prompt());

    // built-in view collections
    app.create('templates');
  };

  /**
   * Initialize listeners
   */

  Generate.initGenerateListeners = function(app) {
    function logger() {
      if (utils.argv.silent || app.options.silent) return;
      console.log.apply(console, arguments);
    }

    app.on('build', function(event, build) {
      if (build && event === 'starting' || event === 'finished') {
        if (build.isSilent) return;
        var prefix = event === 'finished' ? utils.log.success + ' ' : '';
        var key = build.key.replace(/generate\./, '');
        logger(utils.log.timestamp, event, key, prefix + utils.log.red(build.time));
      }
    });

    app.on('task', function(event, task) {
      if (task && task.app) {
        task.app.cwd = app.base.cwd;
      }
      if (task && event === 'starting' || event === 'finished') {
        if (task.isSilent) return;
        var key = task.key.replace(/generate\./, '');
        logger(utils.log.timestamp, event, key, utils.log.red(task.time));
      }
    });

    app.on('task:starting', function(event, task) {
      if (task && task.app) task.app.cwd = app.base.cwd;
    });

    app.on('option', function(key, val) {
      if (key === 'dest') {
        app.base.cwd = val;
        app.cwd = val;
        app.data('dest', val);
      }
    });

    app.on('generator', function(generator) {
      generator.data('alias', generator.env.alias || generator.cache.data.alias);
    });

    app.on('ask', function(val, key, question, answers) {
      if (typeof val === 'undefined') {
        val = question.default;
      }
      if (typeof val === 'undefined') {
        val = question.default = app.cache.answers[key] || app.common.get(key);
      }
      if (typeof val !== 'undefined') {
        app.base.data(key, val);
      }
    });

    app.on('unresolved', function(search, app) {
      if (!/(verb-)?generate-/.test(search.name)) return;
      var opts = {cwd: utils.gm};
      var resolved = utils.resolve.file(search.name, opts) || utils.resolve.file(search.name);
      if (resolved) {
        search.app = app.register(search.name, require(resolved.path));
      }
    });
  };

  /**
   * Temporary error handler method. until we implement better errors.
   *
   * @param {Object} `err` Object or instance of `Error`.
   * @return {Object} Returns an error object, or emits `error` if a listener exists.
   */

  Generate.handleErr = function(app, err) {
    if (!(err instanceof Error)) {
      err = new Error(err.toString());
    }

    if (utils.isObject(app) && app.isApp) {
      if (app.options.verbose) {
        err = err.stack;
      }

      if (app.hasListeners('error')) {
        app.emit('error', err);
      } else {
        throw err;
      }
    } else {
      throw err;
    }
  };

  /**
   * Custom lookup function for resolving generators
   */

  Generate.lookup = function(key) {
    if (!/generate-/.test(key) && key !== 'default') {
      return [`generate-${key}`];
    }
    return [key];
  };
};
