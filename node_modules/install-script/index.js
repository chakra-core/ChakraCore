'use strict';

/**
 * Dependencies
 */

var readFile = require('fs').readFileSync;
var join = require('path').join;
var ejs = require('ejs');


/**
 * Expose `install-script`
 */

module.exports = installScript;


/**
 * Template
 */

const template = readFile(join(__dirname, 'template.ejs'), 'utf-8');


/**
 * Generate install scripts for your CLIs
 */

function installScript (options) {
  return ejs.render(template, options);
}
