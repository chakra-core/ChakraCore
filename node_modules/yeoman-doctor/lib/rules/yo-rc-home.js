'use strict';
const fs = require('fs');
const path = require('path');
const userHome = require('user-home');
const message = require('../message');

exports.description = 'No .yo-rc.json file in home directory';

const errors = {
  fileExists() {
    const rm = process.platform === 'win32' ? 'del' : 'rm';
    return message.get('yo-rc-home-file-exists', {
      yorc: '.yo-rc.json',
      command: rm + ' ~/.yo-rc.json'
    });
  }
};
exports.errors = errors;

exports.yorcPath = path.join(userHome, '.yo-rc.json');

exports.verify = cb => {
  fs.exists(this.yorcPath, exists => {
    cb(exists ? errors.fileExists() : null);
  });
};
