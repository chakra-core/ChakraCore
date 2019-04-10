'use strict';

const path = require('path');
const fs = require('fs');
const addToPATH = require('./add-to-PATH');

module.exports = function envWithLocalPath(config) {
  let configPath = path.join(config.cwd(), 'node_modules', '.bin');
  let modulesPath;

  if (fs.existsSync(configPath)) {
    modulesPath = configPath;
  } else {
    modulesPath = path.join(process.cwd(), 'node_modules', '.bin');
  }
  return addToPATH(modulesPath);
};

module.exports.PATH = addToPATH.PATH;
