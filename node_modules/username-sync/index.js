'use strict';

var os = require('os');
var execSync = require('child_process').execSync;

module.exports = function() {
  return module.exports.os() || module.exports.env() || module.exports.execSync();
};

module.exports.env = function() {
  var env = process.env;

  return env.SUDO_USER ||
    env.C9_USER /* Cloud9 */ ||
    env.LOGNAME ||
    env.USER ||
    env.LNAME ||
    env.USERNAME;
};

module.exports.os = function() {
  try {
    if (typeof os.userInfo === 'function') {
      return os.userInfo().username;
    }
  } catch(e) {
    handleUserInfoError(e);
    return;
  }
};

module.exports.execSync = function() {
  try {
    var username = require('child_process').execSync('whoami').toString().trim();

    if (username) {
      if (process.platform === 'win32') {
        return username = username.replace(/^.*\\/, ''); // remove DOMAIN stuff
      }

      return username;
    }

    return require('child_process').execSync('id', ['-un']).toString().trim();
  } catch(e) {
    handleUserInfoError(e);
    return;
  }
};

function handleUserInfoError(e) {
  if (e !== null && typeof e === 'object' && e.code === 'ENOENT') {
    // if this is run inside a container such as docker, it will fail to get userinfo()
  } else {
   throw (e);
  }
};