'use strict';

const path = require('path');
const ensurePosix = require('ensure-posix-path');
const { moduleResolve } = require('amd-name-resolver');

const BASE_DIR = path.resolve(`${__dirname}/..`);

function getRelativeModulePath(modulePath) {
  return ensurePosix(path.relative(process.cwd(), modulePath));
}

function resolveRelativeModulePath(name, child) {
  return moduleResolve(name, getRelativeModulePath(child));
}

module.exports = {
  getRelativeModulePath,
  resolveRelativeModulePath
};

Object.keys(module.exports).forEach((key) => {
  module.exports[key].baseDir = () => BASE_DIR;

  module.exports[key]._parallelBabel = {
    requireFile: __filename,
    useMethod: key
  };
});
