#!/usr/bin/env node

var path = require('path');
var argv = require('minimist')(process.argv.slice(2));
var writeJson = require('write-json');
var Normalizer = require('./');
var config = new Normalizer();

/**
 * Optionally specific a destination path
 */

var dest = argv.dest || argv.d || 'package.json';

/**
 * Write the file
 */

writeJson(dest, config.normalize(), function(err) {
  if (err) {
    console.error(err);
    process.exit(1);
  }
  console.log('done');
});
