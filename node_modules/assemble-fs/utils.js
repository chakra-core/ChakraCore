'use strict';

var path = require('path');
var utils = require('lazy-cache')(require);
var fn = require;
require = utils;

/**
 * Lazily required module dependencies
 */

require('assemble-handle', 'handle');
require('extend-shallow', 'extend');
require('fs-exists-sync', 'exists');
require('file-is-binary', 'isBinary');
require('is-valid-app');
require('stream-combiner', 'combine');
require('through2', 'through');
require('vinyl-fs', 'vfs');
require = fn;

/**
 * This function does all of the path-specific operations that
 * `prepareWrite` does in http://github.com/wearefractal/vinyl-fs,
 * but on a **cloned file**, which accomplishes two things:
 *
 * 1. We can merge the dest path information onto the context so
 * that it can be used to calculate relative paths for navigation,
 * pagination, etc.
 * 2. Since we use a cloned file, we're not risking any double-processing
 * on the actual view when it's finally written to the file system
 * by the `.dest()` method.
 *
 * @param {Object} view
 * @param {String|Function} dest
 * @param {Object} options
 */

utils.prepare = function(view, dest, options) {
  var file = view.clone();
  var opts = utils.extend({cwd: process.cwd()}, options);
  var cwd = path.resolve(opts.cwd);

  var destDir = typeof dest === 'function' ? dest(file) : dest;
  if (typeof destDir !== 'string') {
    throw new TypeError('expected destination directory to be a string');
  }

  var baseDir = typeof opts.base === 'function'
    ? opts.base(file)
    : path.resolve(cwd, destDir);

  if (typeof baseDir !== 'string') {
    throw new TypeError('expected base directory to be a string');
  }

  var writePath = path.resolve(baseDir, file.relative);
  var data = {};

  data.cwd = cwd;
  data.base = baseDir;
  data.dest = destDir;
  data.path = writePath;
  return data;
};

/**
 * This sets up an event listener that will eventually
 * be called by `app.renderFile()`, ensuring that `dest`
 * information is loaded onto the context before rendering,
 * so that views can render relative paths.
 */

utils.prepareDest = function _(app, dest, options) {
  app.emit('dest', dest, options);

  var appOpts = utils.extend({}, this.options);
  delete appOpts.tasks;
  delete appOpts.engine;

  var opts = utils.extend({}, appOpts, options);
  if (_.prepare) {
    app.off('_prepare', _.prepare);
  }

  _.prepare = function(view) {
    var data = utils.prepare(view, dest, opts);
    view.data = utils.extend({}, view.data, data);
  };

  app.on('_prepare', _.prepare);
};

/**
 * Expose `utils`
 */

module.exports = utils;
