'use strict';
const fs = require('fs');
const path = require('path');
const childProcess = require('child_process');
const message = require('../message');

exports.description = 'NODE_PATH matches the npm root';

const errors = {
  npmFailure() {
    return message.get('node-path-npm-failure', {});
  },

  pathMismatch(npmRoot) {
    let msgPath = 'node-path-path-mismatch';

    if (process.platform === 'win32') {
      msgPath += '-windows';
    }

    return message.get(msgPath, {
      path: process.env.NODE_PATH,
      npmroot: npmRoot
    });
  }
};
exports.errors = errors;

function fixPath(filepath) {
  let fixedPath = path.resolve(path.normalize(filepath.trim()));

  try {
    fixedPath = fs.realpathSync(fixedPath);
  } catch (err) {
    if (err.code !== 'ENOENT' && err.code !== 'ENOTDIR') {
      throw err;
    }
  }

  return fixedPath;
}

exports.verify = cb => {
  if (process.env.NODE_PATH === undefined) {
    cb(null);
    return;
  }

  const nodePaths = (process.env.NODE_PATH || '').split(path.delimiter).map(fixPath);

  childProcess.exec('npm -g root --silent', (err, stdout) => {
    if (err) {
      cb(errors.npmFailure());
      return;
    }

    const npmRoot = fixPath(stdout);

    if (nodePaths.indexOf(npmRoot) < 0) {
      cb(errors.pathMismatch(npmRoot));
      return;
    }

    cb(null);
  });
};
