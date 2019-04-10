'use strict';
const path = require('path');
const fs = require('fs');
const userHome = require('user-home');
const message = require('../message');

exports.description = 'Global configuration file is valid';

const errors = {
  syntax(err, configPath) {
    return message.get('global-config-syntax', {
      message: err.message,
      path: configPath
    });
  },

  misc(configPath) {
    return message.get('global-config-misc', {
      path: configPath
    });
  }
};
exports.errors = errors;

exports.configPath = path.join(userHome, '.yo-rc-global.json');

exports.verify = cb => {
  if (!fs.existsSync(this.configPath)) {
    cb(null);
    return;
  }

  try {
    JSON.parse(fs.readFileSync(this.configPath, 'utf8'));
  } catch (err) {
    if (err instanceof SyntaxError) {
      cb(errors.syntax(err, this.configPath));
      return;
    }

    cb(errors.misc(this.configPath));
    return;
  }

  cb(null);
};
