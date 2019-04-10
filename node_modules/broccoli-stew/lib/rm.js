'use strict';

const Funnel = require('broccoli-funnel');
const debug = require('debug')('broccoli-stew:rm');

/**
 * remove files from a tree.
 *
 * @example
 * const rm = require('broccoli-stew').rm;
 * const dist = 'lib';
 *
 * given:
 *   foo/bar/baz.js
 *   foo/bar/bar.js
 *   foo/bar/package.json
 *   foo/other/package.json
 *
 * tree = 'foo';
 *
 * dist = rm(tree, 'foo/bar/baz.js') => [
 *   foo/bar/bar.js
 *   foo/bar/package.json
 *   foo/other/package.json
 * ];
 *
 * dist = rm(tree, 'foo/bar/*.js') => [
 *   foo/bar/baz.js
 *   foo/bar/bar.js
 * ];
 *
 * dist = rm(tree, 'foo/bar/*.js', 'foo/bar/baz.js') => [
 *   foo/bar/bar.js
 * ];
 *
 * module.exports = dist;
 *
 * @param  {String|Object} tree The input tree
 * @param  {String} [remove]   Pattern to match files to remove
 * @return {Tree}      Tree containing the removed files.
 */
module.exports = function rm(tree/*, ...exclude */) {
  let exclude = Array.prototype.slice.call(arguments, 1, arguments.length);
  debug('%s rm: %s', exclude);

  return new Funnel(tree, { exclude });
};
