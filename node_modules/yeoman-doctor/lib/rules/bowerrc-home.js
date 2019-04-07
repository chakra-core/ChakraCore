'use strict';
const fs = require('fs');
const path = require('path');
const userHome = require('user-home');
const message = require('../message');

exports.description = 'No .bowerrc file in home directory';

const errors = {
  fileExists() {
    const rm = process.platform === 'win32' ? 'del' : 'rm';
    return message.get('bowerrc-home-file-exists', {
      bowerrc: '.bowerrc',
      command: rm + ' ~/.bowerrc'
    });
  }
};
exports.errors = errors;

exports.bowerrcPath = path.join(userHome, '.bowerrc');

exports.verify = cb => {
  fs.exists(this.bowerrcPath, exists => {
    cb(exists ? errors.fileExists() : null);
  });
};
