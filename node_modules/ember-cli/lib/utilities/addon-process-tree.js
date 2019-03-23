'use strict';

module.exports = function addonProcessTree(projectOrAddon, hook, processType, tree) {
  return projectOrAddon.addons.reduce((workingTree, addon) => {
    if (addon[hook]) {
      return addon[hook](processType, workingTree);
    }

    return workingTree;
  }, tree);
};
