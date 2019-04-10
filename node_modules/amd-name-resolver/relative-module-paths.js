'use strict';

const path = require('path');
const ensurePosix = require('ensure-posix-path');
const { moduleResolve } = require('./index');

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
  module.exports[key].baseDir = () => __dirname;

  module.exports[key]._parallelBabel = {
    requireFile: __filename,
    useMethod: key
  };
});
