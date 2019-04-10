/*!
 * assemble-render-file <https://github.com/assemble/assemble-render-file>
 *
 * Copyright (c) 2015-2017, Jon Schlinkert.
 * Released under the MIT License.
 */

'use strict';

var PluginError = require('plugin-error');
var utils = require('./utils');

/**
 * Render a vinyl file.
 *
 * ```js
 * app.src('*.hbs')
 *   .pipe(app.renderFile());
 * ```
 *
 * @name .renderFile
 * @param  {Object} `locals` Optional locals to pass to the template engine for rendering.
 * @return {Object}
 * @api public
 */

module.exports = function(config) {
  return function plugin(app) {
    if (!utils.isValid(app, 'assemble-render-file', ['app', 'collection'])) return;
    var opts = utils.merge({}, app.options, config);
    var debug = utils.debug;

    app.define('renderFile', function(engine, locals) {
      if (typeof engine !== 'string') {
        locals = engine;
        engine = null;
      }

      debug('renderFile: engine "%s"', engine);

      locals = locals || {};
      var collection = {};

      if (locals && !locals.isCollection) {
        opts = utils.merge({}, opts, locals);
      } else {
        collection = locals;
        locals = {};
      }

      var View = opts.View || opts.File || collection.View || this.View;
      var files = [];

      return utils.through.obj(function(file, enc, next) {
        var stream = this;

        if (file.isNull()) {
          next(null, file);
          return;
        }

        if (utils.isBinary(file)) {
          next(null, file);
          return;
        }

        if (file.data.render === false || opts.render === false) {
          next(null, file);
          return;
        }

        if (!file.isView) file = new View(file);
        files.push(file);

        app.handleOnce('onLoad', file, function(err) {
          if (err) {
            handleError(app, err, file, files, next);
            return;
          }

          app.emit('_prepare', file);
          next();
        });

      }, function(cb) {
        var stream = this;

        // run `onLoad` middleware
        utils.reduce(files, [], function(acc, file, next) {
          debug('renderFile, preRender: %s', file.path);

          resolveEngine(app, locals, engine);

          if (!locals.engine && app.isFalse('engineStrict')) {
            stream.push(file);
            next();
            return;
          }

          // render the file
          app.render(file, locals, function(err, res) {
            if (typeof res === 'undefined' || err) {
              handleError(app, err, file, files, next);
              return;
            }

            debug('renderFile, postRender: %s', file.relative);
            stream.push(res);
            next();
          });
        }, cb);
      });
    });

    return plugin;
  };
};

function handleError(app, err, view, files, next) {
  var last = files[files.length - 1];
  var errOpts = {fileName: last.path, showStack: true};
  if (!err || !err.message) {
    err = 'view cannot be rendered';
  }
  err = new PluginError('assemble-render-file', err, errOpts);
  err.files = files;
  err.view = last;
  next(err);
}

function resolveEngine(app, locals, engine) {
  if (typeof engine === 'string') {
    locals.engine = engine;
    return;
  }
  if (locals.engine) {
    return;
  }
  // allow a `noop` engine to be defined
  if (app.engines['.noop']) {
    locals.engine = '.noop';
  }
}
