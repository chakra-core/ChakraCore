'use strict';

const cloneDeep = require('lodash.clonedeep');
const isWin = require('./utils/is-win')();
let PATH = 'PATH';
let delimiter = ':';

// windows calls it's path 'Path' usually, but this is not guaranteed.
if (isWin) {
  PATH = 'Path';
  delimiter = ';';
  Object.keys(process.env).forEach(function(e) {
    if (e.match(/^PATH$/i)) {
      PATH = e;
    }
  });
}

module.exports = function addToPATH(path) {
  let env = cloneDeep(process.env);
  env[PATH] = [path, env[PATH]].join(delimiter);

  return env;
};

module.exports.PATH = PATH;
