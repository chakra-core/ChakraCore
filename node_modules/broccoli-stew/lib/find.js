'use strict';

const Funnel = require('broccoli-funnel');
const decompose = require('./utils/decompose');
const debug = require('debug')('broccoli-stew:find');
const merge = require('broccoli-merge-trees');

/**
 * Allows you to find files in a tree via a glob patterns.
 *
 * @example
 *
 * given:
 *   root/package.json
 *   root/lib/bar.js
 *   root/lib/foo.js
 *   root/lib/bar.coffee
 *   root/lib/bar.txt
 *
 * let tree = 'root';
 *
 * find(tree) => identity;
 *
 * find('root/lib/*.js') => [
 *   'root/lib/bar.js'
 *   'root/lib/foo.js'
 * ];
 *
 * find(tree, '*.js') => [
 *   'lib/bar.js'
 *   'lib/foo.js'
 * ];
 *
 * find(tree, 'lib/*.{coffee.js}') => [
 *   'lib/bar.js',
 *   'lib/bar.coffee',
 *   'lib/foo.js',
 * ];
 *
 * find(tree, function(path) {
 *   return /*.js/.test(path);
 * }) => [
 *   'lib/bar.js',
 *   'lib/foo.js',
 * ];
 *
 * find(tree, { include: ['*.js'], exclude: [ 'bar.*' ]) => [
 *  'lib/foo.js'
 * ]);
 *
 * find(tree, { include: [/*.js/], exclude: [ 'bar.*' ]) => [
 *  'lib/foo.js'
 * ]);
 *
 * also given:
 *   other/foo/bar.jcs
 *   other/foo/bar.css
 *   other/foo/package.json
 *
 * other = 'other';
 *
 * find([
 *  root,
 *  other
 *]) => [
 *   package.json
 *   lib/bar.js
 *   lib/foo.js
 *   lib/bar.coffee
 *   lib/bar.txt
 *   foo/bar.jcs
 *   foo/bar.css
 * ]
 **
 * find([
 *  root,
 *  other
 * ],'**//*.js') => [
 *   lib/bar.js
 *   lib/foo.js
 * ]
 *
 * find([
 *  root,
 *  other
 * ],{ overwrite: true }') => [
 *   lib/bar.js
 *   lib/foo.js
 * ]
 *
 * @name find
 * @argument tree
 * @argument filter
 */
module.exports = function find(tree, a, b) {
  let arity = arguments.length;

  if (Array.isArray(tree)) {
    let mergeOptions;
    if (a&& typeof a === 'object' &&a.overwrite) {
      mergeOptions = {
        overwrite: true
      };
    }
    return merge(tree.map(function(tree) {
      return _find.apply(null, [tree, a, b].filter(Boolean));
    }), mergeOptions);
  } else {
    return _find.apply(null, [tree, a, b].filter(Boolean));
  }
};

function _find(tree, _options) {
  let options = {};
  let root = tree;

  if (arguments.length === 1 && typeof root === 'string') {
    let decomposition = decompose(root.replace(/[a-z]:[\\\/]/i,'/'));

    root = decomposition.root;

    options.destDir = root;
    options.include = decomposition.include;
    options.exclude = decomposition.exclude;
  } else if (arguments.length > 1) {

    if (typeof _options === 'object') {
      options = _options;
    } else if (arguments.length > 1 && typeof arguments[2] === 'object') {
      options = arguments[2];
    }

    if (typeof _options === 'string') {

      let pattern = decompose.expandSingle(_options);
      let lastChar = pattern.charAt(pattern.length - 1);

      if (typeof root === 'string') {
        options.destDir = root;
      } else if (root.inputTree && typeof root.inputTree.destDir === 'string') {
        options.destDir = root.inputTree.destDir;
      }

      if (lastChar === '*' || lastChar !== '/') {
        options.include = [pattern];
      } else {
        options.include = [pattern + '**/*'];
      }
    } else {
      options = _options;
    }
  }

  debug('%s include: %o exclude: %o destDir: %s', root, options.include, options.exclude, options.destDir);

  return new Funnel(root, options);
}
