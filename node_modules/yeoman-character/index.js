'use strict';
var path = require('path');
var fs = require('fs');
var supportsColor = require('supports-color');

var fallback = [
	'    _-----_',
	'   |       |',
	'   |--(o)--|',
	'  `---------´',
	'   ( _´U`_ )',
	'   /___A___\\',
	'    |  ~  |',
	'  __\'.___.\'__',
	'´   `  |° ´ Y `'
].join('\n');

module.exports = supportsColor && process.platform !== 'win32' ?
	fs.readFileSync(path.join(__dirname, 'yeoman.txt'), 'utf8') : fallback;
