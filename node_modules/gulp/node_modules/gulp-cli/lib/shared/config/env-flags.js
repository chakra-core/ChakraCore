'use strict';

var path = require('path');
var copyProps = require('copy-props');

var toFrom = {
  configPath: 'flags.gulpfile',
  configBase: 'flags.gulpfile',
};

function mergeConfigToEnvFlags(env, config) {
  return copyProps(env, config, toFrom, convert, true);
}

function convert(configInfo, envInfo) {
  if (envInfo.keyChain === 'configBase') {
    return path.dirname(configInfo.value);
  }
  return configInfo.value;
}

module.exports = mergeConfigToEnvFlags;
