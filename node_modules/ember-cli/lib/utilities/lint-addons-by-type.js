'use strict';

module.exports = function lintAddonsByType(addons, type, tree) {
  return addons.reduce((sum, addon) => {
    if (addon.lintTree) {
      let val = addon.lintTree(type, tree);
      if (val) { sum.push(val); }
    }
    return sum;
  }, []);
};
