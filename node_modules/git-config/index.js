var parser = require('iniparser'),
    path = require('path');

module.exports = function (gitConfigPath, cb) {
  if (typeof cb === 'undefined') {
    cb = gitConfigPath;
    gitConfigPath = path.join(
      process.env.HOME || process.env.USERPROFILE, '.gitconfig');
  }
  parser.parse(gitConfigPath, cb);
};

module.exports.sync = function (gitConfigPath) {
  if (typeof gitConfigPath === 'undefined') {
    gitConfigPath = path.join(
      process.env.HOME || process.env.USERPROFILE, '.gitconfig');
  }
  var results = {};
  try {
    results = parser.parseSync(gitConfigPath);
  } catch (err) { }
  return results;
};
