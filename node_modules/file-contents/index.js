'use strict';

var fs = require('fs');
var utils = require('./utils');

/**
 * Add the `contents` property to a `file` object.
 *
 * @param  {Object} `options`
 * @return {Object} File object.
 * @api public
 */

module.exports = function(options) {
  return utils.through.obj(function(file, enc, next) {
    contentsAsync(file, options, next);
  });
};

/**
 * Async method for getting `file.contents`.
 *
 * @param  {Object} `file`
 * @param  {Object} `options`
 * @param  {Function} `cb`
 * @return {Object}
 */

function contentsAsync(file, options, cb) {
  if (typeof options === 'function') {
    cb = options;
    options = {};
  }

  if (typeof cb !== 'function') {
    throw new TypeError('expected a callback function');
  }

  if (!utils.isObject(file)) {
    cb(new TypeError('expected file to be an object'));
    return;
  }

  contentsSync(file, options);
  cb(null, file);
}

/**
 * Sync method for getting `file.contents`.
 *
 * @param  {Object} `file`
 * @param  {Object} `options`
 * @return {Object}
 */

function contentsSync(file, options) {
  if (!utils.isObject(file)) {
    throw new TypeError('expected file to be an object');
  }

  try {
    // ideally we want to stat the real, initial filepath
    file.stat = fs.lstatSync(file.history[0]);
  } catch (err) {
    try {
      // if that doesn't work, try again
      file.stat = fs.lstatSync(file.path);
    } catch (err) {
      file.stat = new fs.Stats();
    }
  }

  utils.syncContents(file, options);
  return file;
}

/**
 * Expose `async` method
 */

module.exports.async = contentsAsync;

/**
 * Expose `sync` method
 */

module.exports.sync = contentsSync;
