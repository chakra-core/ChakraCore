'use strict';

var path = require('path');
var utils = require('./utils');
var list = require('./list');

module.exports = function(app, base, env, options) {
  var gm = path.resolve.bind(path, utils.gm);
  var cwd = path.resolve.bind(path, app.cwd);

  /**
   * Register sub-generators (as plugins)
   */

  app.use(require('generate-collections'));
  app.use(require('generate-defaults'));

  /**
   * Generate an `assemblefile.js`
   */

  app.task('new', function(cb) {
    app.src('templates/generator.js', {cwd: __dirname})
      .pipe(app.dest(app.options.dest || app.cwd))
      .on('end', function() {
        console.log(utils.log.timestamp, 'created generator.js');
        cb();
      });
  });

  /**
   * Display a list of installed generators.
   *
   * ```sh
   * $ gen list
   * ```
   * @name list
   * @api public
   */

  app.task('list', { silent: true }, function() {
    return app.src([gm('generate-*'), cwd('node_modules/generate-*')])
      .pipe(utils.through.obj(function(file, enc, next) {
        file.alias = app.toAlias(file.basename);
        next(null, file);
      }))
      .pipe(list(app));
  });

  app.task('tasks', { silent: true }, function(cb) {
    Object.keys(app.tasks).forEach(function(name) {
      console.log(' · ' + app.log.cyan(name));
    });
    cb();
  });

  /**
   * Display `--help`.
   *
   * ```sh
   * $ gen defaults:help
   * ```
   * @name help
   * @api public
   */

  app.task('help', { silent: true }, function(cb) {
    app.enable('silent');
    app.cli.process({ help: true }, cb);
  });

  /**
   * Render a single `--src` file to the given `--dest` or current working directory.
   *
   * ```sh
   * $ gen defaults:render
   * # aliased as
   * $ gen render
   * ```
   * @name render
   * @api public
   */

  app.task('render', function(cb) {
    if (!app.option('src')) {
      app.emit('error', new Error('Expected a `--src` filepath'));
    } else if (!app.option('dest')) {
      app.build(['dest', 'render'], cb);
    } else {
      file(app, app.option('src'), app.cwd, cb);
    }
  });

  /**
   * Default task
   */

  app.task('default', ['help']);
};

function file(app, name, templates, dest, cb) {
  if (typeof dest === 'function') {
    cb = dest;
    dest = null;
  }

  dest = dest || app.option('dest') || '';
  app.src(name, {cwd: templates})
    .pipe(app.renderFile('*'))
    .pipe(app.conflicts(dest))
    .pipe(app.dest(dest))
    .on('error', cb)
    .on('end', cb);
}
