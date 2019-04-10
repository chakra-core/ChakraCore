'use strict';

const BroccoliDebug = require('broccoli-debug');

/**
 * Writes the passed tree to disk at the root of
 * the project.
 *
 * @example
 * const myTree = someBroccoliPlugin('lib');
 *
 * myTree = debug(myTree);
 *
 * module.exports = myTree;
 *
 * @param  {String|Object} tree    The input tree to debug.
 * @param  {Object} options
 * @property {String} options.name The name of directory you want to write to
 */
module.exports = function debug(tree, labelOrOptions) {
  let options;
  if (typeof labelOrOptions === 'string') {
    options = {
      name: labelOrOptions
    };
  } else {
    options = labelOrOptions;
  }

  if (!options) {
    options = {};
  }

  if (!options.name && tree) {
    options.name = tree.toString().replace(/[^a-z0-9]/gmi, '_');
  }

  if (!options.name) {
    throw Error('Must pass folder name to write to. stew.debug(tree, { name: "foldername" })');
  }

  let broccoliDebugOptions = {
    label: options.name,
    force: true
  };

  if (options.dir) {
    broccoliDebugOptions.baseDir = options.dir;
  }

  return new BroccoliDebug(tree, broccoliDebugOptions);
};
