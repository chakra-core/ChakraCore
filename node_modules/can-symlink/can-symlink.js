#!/usr/bin/env node
var canSymlink = require('./');

if (canSymlink()) {
	console.log('Able to create symlinks.');
} else {
	console.log('Unable to create symlinks! Make sure your shell is running with the appropriate permissions.');
}