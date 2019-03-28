'use strict';

var path = require('path');
var utils = require('./utils');

/**
 * Return the dirname for the given `filepath`. Uses
 * the node.js [path] module.
 *
 * ```js
 * <%= dirname("a/b/c/d") %>
 * //=> 'a/b/c'
 * ```
 *
 * @param {String} `filepath`
 * @return {String} Returns the directory part of the file path.
 * @api public
 */

exports.dirname = function dirname(filepath) {
  return path.dirname(filepath);
};

/**
 * Return the basename for the given `filepath`. Uses
 * the node.js [path] module.
 *
 * ```js
 * <%= basename("a/b/c/d.js") %>
 * //=> 'd.js'
 * ```
 *
 * @param {String} `filepath`
 * @return {String} Returns the basename part of the file path.
 * @api public
 */

exports.basename = function basename(fp) {
  return path.basename(fp);
};

/**
 * Return the filename for the given `filepath`, excluding
 * extension.
 *
 * ```js
 * <%= basename("a/b/c/d.js") %>
 * //=> 'd'
 * ```
 *
 * @param {String} `filepath`
 * @return {String} Returns the file name part of the file path.
 * @api public
 */

exports.filename = function filename(filepath) {
  return path.basename(filepath, path.extname(filepath));
};

/**
 * Return the file extension for the given `filepath`.
 * Uses the node.js [path] module.
 *
 * ```js
 * <%= extname("foo.js") %>
 * //=> '.js'
 * ```
 *
 * @param {String} `filepath`
 * @return {String} Returns a file extension
 * @api public
 */

exports.extname = function extname(filepath) {
  return path.extname(filepath);
};

/**
 * Return the file extension for the given `filepath`,
 * excluding the `.`.
 *
 * ```js
 * <%= ext("foo.js") %>
 * //=> 'js'
 * ```
 *
 * @param {String} `filepath`
 * @return {String} Returns a file extension without dot.
 * @api public
 */

exports.ext = function ext(filepath) {
  return path.extname(filepath).slice(1);
};

/**
 * Resolves the given paths to an absolute path. Uses
 * the node.js [path] module.
 *
 * ```js
 * <%= resolve('/foo/bar', './baz') %>
 * //=> '/foo/bar/baz'
 * ```
 *
 * @param {String} `filepath`
 * @return {String} Returns a resolve
 * @api public
 */

exports.resolve = function resolve() {
  return path.resolve.apply(path, arguments);
};

/**
 * Get the relative path from file `a` to file `b`.
 * Typically `a` and `b` would be variables passed
 * on the context. Uses the node.js [path] module.
 *
 * ```js
 * <%= relative(a, b) %>
 * ```
 *
 * @param {String} `a` The "from" file path.
 * @param {String} `b` The "to" file path.
 * @return {String} Returns a relative path.
 * @api public
 */

exports.relative = function relative(a, b) {
  if (typeof b === 'undefined' && typeof process !== 'undefine' && typeof process.cwd === 'function') {
    b = a;
    a = process.cwd();
  }
  a = a || '';
  b = b || '';
  var rel = utils.relative(a, b);
  return rel === '.' ? './' : rel;
};

/**
 * Get specific (joined) segments of a file path by passing a
 * range of array indices.
 *
 * ```js
 * <%= segments("a/b/c/d", "2", "3") %>
 * //=> 'c/d'
 *
 * <%= segments("a/b/c/d", "1", "3") %>
 * //=> 'b/c/d'
 *
 * <%= segments("a/b/c/d", "1", "2") %>
 * //=> 'b/c'
 * ```
 *
 * @param {String} `filepath` The file path to split into segments.
 * @return {String} Returns a single, joined file path.
 * @api public
 */

exports.segments = function segments(filepath, a, b) {
  return filepath.split(/[\\\/]+/).slice(a, b).join('/');
};

/**
 * Join all arguments together and normalize the resulting
 * `filepath`. Uses the node.js [path] module.
 *
 * **Note**: there is also a `join()` array helper, dot notation
 * can be used with helpers to differentiate. Example: `<%= path.join() %>`.
 *
 *
 * ```js
 * <%= join("a", "b") %>
 * //=> 'a/b'
 * ```
 *
 * @param {String} `filepaths` List of file paths.
 * @return {String} Returns a single, joined file path.
 * @api public
 */

exports.join = function join() {
  return path.join.apply(path, arguments);
};

/**
 * Returns true if a file path is an absolute path. An
 * absolute path will always resolve to the same location,
 * regardless of the working directory. Uses the node.js
 * [path] module.
 *
 * ```js
 * // posix
 * <%= isAbsolute('/foo/bar') %>
 * //=> 'true'
 * <%= isAbsolute('qux/') %>
 * //=> 'false'
 * <%= isAbsolute('.') %>
 * //=> 'false'
 *
 * // Windows
 * <%= isAbsolute('//server') %>
 * //=> 'true'
 * <%= isAbsolute('C:/foo/..') %>
 * //=> 'true'
 * <%= isAbsolute('bar\\baz') %>
 * //=> 'false'
 * <%= isAbsolute('.') %>
 * //=> 'false'
 * ```
 *
 * @param {String} `filepath`
 * @return {String} Returns a resolve
 * @api public
 */

exports.isAbsolute = function isAbsolute(filepath) {
  return utils.isAbsolute(filepath);
};

/**
 * Returns true if a file path is an absolute path. An
 * absolute path will always resolve to the same location,
 * regardless of the working directory. Uses the node.js
 * [path] module.
 *
 * ```js
 * // posix
 * <%= isRelative('/foo/bar') %>
 * //=> 'false'
 * <%= isRelative('qux/') %>
 * //=> 'true'
 * <%= isRelative('.') %>
 * //=> 'true'
 *
 * // Windows
 * <%= isRelative('//server') %>
 * //=> 'false'
 * <%= isRelative('C:/foo/..') %>
 * //=> 'false'
 * <%= isRelative('bar\\baz') %>
 * //=> 'true'
 * <%= isRelative('.') %>
 * //=> 'true'
 * ```
 *
 * @param {String} `filepath`
 * @return {String} Returns a resolve
 * @api public
 */

exports.isRelative = function isRelative(filepath) {
  return !utils.isAbsolute(filepath);
};
