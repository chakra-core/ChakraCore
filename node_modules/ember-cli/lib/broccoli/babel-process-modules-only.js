'use strict';

module.exports = function processModulesOnly(tree, annotation) {
  const ResolveModulePaths = require('amd-name-resolver/relative-module-paths');

  let options = {
    plugins: [
      [require.resolve('babel-plugin-module-resolver'), { resolvePath: ResolveModulePaths.resolveRelativeModulePath }],
      [require.resolve('@babel/plugin-transform-modules-amd'), { noInterop: true }],
    ],
    moduleIds: true,
    getModuleId: ResolveModulePaths.getRelativeModulePath,
    annotation,
  };

  const Babel = require('broccoli-babel-transpiler');
  return new Babel(tree, options);
};
