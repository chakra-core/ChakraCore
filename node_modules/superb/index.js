'use strict';
var words = require('./words.json');
var uniqueRandomArray = require('unique-random-array');

module.exports = uniqueRandomArray(words);
module.exports.words = words;
