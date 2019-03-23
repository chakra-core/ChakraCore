'use strict';

const Funnel = require('broccoli-funnel');
const debug = require('debug')('broccoli-stew:rename');

/**
 * Renames files in a tree.
 *
 * @example
 * const rename = require('broccoli-stew').rename;
 * const uglify = require('broccoli-uglify-js');
 * const dist = 'lib';
 *
 * dist = rename(uglify(dist), '.js', '.min.js');
 * // or
 * dist = rename(uglify(dist), function(relativePath) {
 *   return relativePath.replace('.js', '.min.js');
 * });
 *
 *
 * module.exports = dist;
 *
 * @param  {String|Object} tree The input tree
 * @param  {String|Function} from This is either the starting replacement pattern or a custom replacement function.
 * @param  {String} [to]   Pattern to replace to
 * @return {Tree}      Tree containing the renamed files.
 */
module.exports = function rename(tree, from, to) {
  let getDestinationPath;

  if (arguments.length === 2 && typeof from === 'function') {
    getDestinationPath = from;
  } else {
    getDestinationPath = relativePath => relativePath.replace(from, to)
  }

  debug('%s from: %s to: %s', tree, from, to);

  return new Funnel(tree, { getDestinationPath });
};
