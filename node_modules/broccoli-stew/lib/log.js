'use strict';

const walkSync   = require('walk-sync');
const Promise    = require('rsvp').Promise;
const chalk      = require('chalk');
const DIR_REGEX  = /\/$/;
const symlinkOrCopy = require('symlink-or-copy');
const Noop = require('./utils/noop');

/**
 * Logs out files in the passed tree to stdout.
 *
 * For the log to be actually printed out, the resulting tree has to be consumed by Broccoli.
 *
 * @example
 *
 * let tree = find('zoo/animals/*.js');
 *
 * tree = log(tree);
 * // [cat.js, dog.js]
 *
 * tree = log(tree, {output: 'tree'});
 * // /Users/chietala/workspace/broccoli-stew/tmp/funnel-dest_Q1EeTD.tmp
 * // ├── cat.js
 * // └── dog.js
 *
 * module.exports = tree;
 *
 * @param  {String|Object} tree    The input tree to debug
 * @param  {Object|String} [options]
 * @property {String} [options.output] Print to stdout as a tree
 * @property {String} [options.label] Will label the the tree in stdout
 */
module.exports = function log(tree, _options) {
  let options = parseOptions(arguments.length, _options);

  options.name = `stew.log ${options.label || ''}`
  options.build = function() {
    print(this.inputPaths[0], this.options);
  }

  return new Noop(tree, options);
};

function print(dir, options) {
  let debugEnv = process.env.DEBUG;
  let label = options.label;
  let debugOnly = options.debugOnly;
  let tree = options.output === 'tree';
  let files = walkSync(dir);
  let debuggr = options.debugger || require('debug');
  let printTree = treeOutput(dir);
  let debug;

  if(typeof debugEnv === 'string') {
    debugEnv = debugEnv.replace(/\*/g, '.*?');
  }

  if (debugOnly) {
    if (new RegExp(debugEnv).test(label)) {
      debug = debuggr(label);
      if (tree) {
        debug(printTree.stdout);
      } else {
        debug(files);
      }
    }
  } else {

    if (label && tree) {
      console.log(label);
      console.log(printTree.stdout);
    } else if (tree && !label) {
      console.log(printTree.stdout);
    } else {
      if (label) {
        console.log(label);
      }
      process.stdout.write(JSON.stringify(files, null, 2) + '\n');
    }
  }
  return dir;
}

function tab(size) {
  let _tab = '   ';
  return new Array(size).join(_tab);
}

function treeOutput(dir) {
  let end = '└── ';
  let ls = '├── ';
  let newLine = '\n';
  let stdout = '';

  let files = walkSync(dir).map((path, i, arr) => {
    let depth = path.split('/').length;
    if (DIR_REGEX.test(path)) {
      stdout += (newLine + tab(depth - 1) + end + path);
    } else {
      stdout += (newLine + tab(depth));
      if (DIR_REGEX.test(arr[i + 1]) || i === (arr.length - 1)) {
        stdout += (end + path);
      } else {
        stdout += (ls + path);
      }
    }
    return path;
  }).filter(path => !DIR_REGEX.test(path));

  return {
    stdout: stdout,
    files: files
  };
}

function parseOptions(arity, options) {
  if (arity === 1) {
    return { };
  } if (typeof options === 'string') {
    return {
      label: options
    };
  } else if (options && typeof options === 'object') {
    return options;
  } else if (arity > 1) {
    throw new TypeError('log(tree, options), `options` is invalid, expected label or configuration, but got: ' + options);
  }
}
