'use strict';

const findup = require('find-up');
const fs = require('fs');
const path = require('path');
const yaml = require('js-yaml');

let parsers = {
  '.yml': yaml.safeLoad,
  '.yaml': yaml.safeLoad,
  '.json': JSON.parse,
};

function loadConfig(file, basedir) {
  basedir = basedir || __dirname;
  let ext = path.extname(file);
  let configPath = findup.sync(file, { cwd: basedir });

  let content;
  try {
    content = fs.readFileSync(configPath, 'utf8');
    if (parsers[ext]) {
      content = parsers[ext](content);
    }
  } catch (e) {
    // ESLint doesn't like this.
  }

  return content;
}

module.exports = loadConfig;
