'use strict';

var fs = require('fs');
var path = require('path');
var exists = require('fs-exists-sync');

/**
 * Return true if a file exists
 *
 * ```js
 * <%= exists("foo.js") %>
 * ```
 *
 * @param {String} `filepath` Path of the file to check.
 * @return {Boolean} True if the file exists
 * @api public
 */

exports.exists = function(filepath) {
  return filepath && exists(filepath);
};

/**
 * Read a file from the file system and inject its content
 *
 * ```js
 * <%= read("foo.js") %>
 * ```
 *
 * @param {String} `filepath` Path of the file to read.
 * @return {String} Contents of the given file.
 * @api public
 */

exports.read = function read(filepath) {
  filepath = path.resolve(filepath);
  if (!exists(filepath)) return '';
  return fs.readFileSync(filepath, 'utf8');
};
