#!/usr/bin/env node

process.title = 'project-name';
var argv = require('minimist')(process.argv.slice(2));
var name = require('./')(argv.cwd || process.cwd());
console.log(name);
