'use strict';

const fs = require('fs');
const findWorkspaceRoot = require('find-yarn-workspace-root');

/**
 * Returns true if and only if the given directory has a yarn.lock file or is a child of a
 * yarn workspace root.
 * @private
 * @method isYarnProject
 * @param {string} thePath
 * @return {boolean}
 */
function isYarnProject(thePath) {
  if (fs.existsSync(`${thePath}/yarn.lock`)) {
    return true;
  }

  if (findWorkspaceRoot(thePath)) {
    return true;
  }

  return false;
}

module.exports = isYarnProject;
