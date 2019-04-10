'use strict';

const fs = require('fs');
const execa = require('execa');
const Bluebird = require('bluebird');
const log = require('npmlog');

const fsStatAsync = Bluebird.promisify(fs.stat);

const isWin = require('./utils/is-win')();

exports.fileExists = function fileExists(path) {
  return fsStatAsync(path).then(stat => stat.isFile()).catchReturn(false);
};

exports.executableExists = function executableExists(exe, options) {
  let cmd = isWin ? 'where' : 'which';
  let test = execa(cmd, [exe], options);

  test.on('error', error => log.error('Error spawning "' + cmd + exe + '"', error));

  return test.then(result => result.code === 0, () => false);
};
