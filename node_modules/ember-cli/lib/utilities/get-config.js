'use strict';

let Yam = require('yam');
let Project = require('../models/project');

function generateConfig() {
  return new Yam('ember-cli', {
    primary: Project.getProjectRoot(),
  });
}

module.exports = function getConfig(override) {
  if (override) {
    return override;
  }

  return generateConfig();
};
