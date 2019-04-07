#!/usr/bin/env node
'use strict';
var meow = require('meow');
var superb = require('./');

var cli = meow([
	'Examples',
	'  $ superb',
	'  legendary',
	'',
	'  $ superb --all',
	'  ace',
	'  amazing',
	'  ...',
	'',
	'Options',
	'  --all  Get all the words instead of a random word'
]);

console.log(cli.flags.all ? superb.words.join('\n') : superb());
