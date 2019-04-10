'use strict';

var path = require('path');
var utils = require('./utils');

module.exports = function(app, base, env) {
  if (!utils.isValid(app, 'generate-install')) {
    return;
  }

  /**
   * Plugins
   */

  app.use(require('generate-defaults'));

  /**
   * Middleware to get `install` objects from front-matter
   */

  if (!skipInstall(app)) {
    app.postWrite(/./, function(file, next) {

      // check again, in case options were updated
      if (skipInstall(app)) {
        next();
        return;
      }

      if (typeof file.data === 'undefined' || typeof file.data.install === 'undefined') {
        next();
        return;
      }

      if (utils.isObject(file.data.install)) {
        for (var type in file.data.install) {
          app.base.union(['cache.install', type], utils.arrayify(file.data.install[type]));
        }
      } else {
        app.base.union('cache.install.devDependencies', utils.arrayify(file.data.install));
      }
      next();
    });
  }

  /**
   * Initiates a prompt to install any dependencies detected during the build.
   *
   * ```sh
   * $ gen install
   * ```
   * @name default
   * @api public
   */

  app.task('default', ['prompt-install']);

  /**
   * Automatically install any `dependencies` or `devDependencies` after writing files to
   * the file system. By default this only installs deps that were found in front-matter.
   *
   * ```sh
   * $ gen install
   * ```
   * @name install
   * @api public
   */

  app.task('install', install(app));

  /**
   * Prompt to install any `dependencies` or `devDependencies` after writing files to
   * the file system. By default this only installs deps that were found in front-matter.
   * This task is semantically named for API usage.
   *
   * ```sh
   * $ gen install:prompt-install
   * ```
   * @name prompt-install
   * @api public
   */

  app.task('prompt', ['prompt-install']);
  app.task('prompt-install', install(app, true));
};

function skipInstall(app) {
  var opts = getOptions(app);
  return opts.install === false || opts['skip-install'] === true;
}

function getOptions(app) {
  return utils.extend({}, app.options, app.option('generator.install'));
}

function install(app, prompt) {
  return function(cb) {
    if (skipInstall(app)) {
      cb();
      return;
    }

    if (!utils.exists(path.resolve(process.cwd(), 'package.json'))) {
      app.log.error('package.json does not exist, cannot install dependencies');
      cb();
      return;
    }

    var types = app.base.get('cache.install') || {};
    if (typeof types === 'undefined') {
      cb();
      return;
    }

    if (Array.isArray(types)) {
      types = { devDependencies: types };
    }

    utils.eachSeries(Object.keys(types), function(type, next) {
      var keys = types[type] || [];
      var names = utils.unique(app.pkg.data, type, keys);
      if (!names.length) {
        next();
        return;
      }
      if (prompt === true) {
        app.log.success('installing', names);
        app.npm.askInstall(names, {method: type, type: type}, next);
      } else {
        app.npm[type](names, next);
      }
    }, cb);
  };
}
