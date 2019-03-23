'use strict';
const upstreamMergeTrees = require('broccoli-merge-trees');
const heimdall = require('heimdalljs');
const heimdallLogger = require('heimdalljs-logger');
const logger = heimdallLogger('ember-cli:merge-trees');
let EMPTY_MERGE_TREE;

function overrideEmptyTree(tree) {
  EMPTY_MERGE_TREE = tree;
}

function getEmptyTree() {
  if (EMPTY_MERGE_TREE) {
    return EMPTY_MERGE_TREE;
  }

  EMPTY_MERGE_TREE = module.exports._upstreamMergeTrees([], {
    annotation: 'EMPTY_MERGE_TREE',
    description: 'EMPTY_MERGE_TREE',
  });

  let originalCleanup = EMPTY_MERGE_TREE.cleanup;
  EMPTY_MERGE_TREE.cleanup = function() {
    // this tree is being cleaned up, we must
    // ensure that our shared EMPTY_MERGE_TREE is
    // reset (otherwise it will not have a valid
    // `outputPath`)
    EMPTY_MERGE_TREE = null;
    return originalCleanup.apply(this, arguments);
  };

  return EMPTY_MERGE_TREE;
}

module.exports = function mergeTrees(_inputTrees, options) {
  options = options || {};

  let node = heimdall.start({
    name: `EmberCliMergeTrees(${options.annotation})`,
    emberCliMergeTrees: true,
  }, function MergeTreesSchema() {
    this.emptyTrees = 0;
    this.duplicateTrees = 0;
    this.returns = {
      empty: 0,
      identity: 0,
      merge: 0,
    };
  });

  let inputTrees = _inputTrees.filter(function _removeEmptyTrees(tree) {
    if (tree && tree !== getEmptyTree()) {
      return true;
    } else {
      node.stats.emptyTrees++;
      logger.debug('Removing empty tree');
      return false;
    }
  }).reduce(function _dedupeTrees(result, tree) {
    // Check for a previous duplicate and remove it so the last one "wins"
    let indexOfTree = result.indexOf(tree);
    if (indexOfTree !== -1) {
      node.stats.duplicateTrees++;
      logger.debug(`Removing duplicate tree: ${tree.annotation}`);
      result.splice(indexOfTree, 1);
    }

    result.push(tree);
    return result;
  }, []);

  switch (inputTrees.length) {
    case 0:
      node.stats.returns.empty++;
      logger.info('Returning empty tree');
      node.stop();
      return getEmptyTree();
    case 1:
      node.stats.returns.identity++;
      logger.info('Returning single input tree');
      node.stop();
      return inputTrees[0];
    default: {
      options.description = options.annotation;
      let tree = module.exports._upstreamMergeTrees(inputTrees, options);

      tree.description = options && options.description;

      node.stats.returns.merge++;
      logger.info('Returning upstream merged trees');
      node.stop();
      return tree;
    }
  }
};

module.exports.isEmptyTree = function isEmptyTree(tree) {
  return tree === EMPTY_MERGE_TREE;
};

module.exports._overrideEmptyTree = overrideEmptyTree;
module.exports._upstreamMergeTrees = upstreamMergeTrees;
