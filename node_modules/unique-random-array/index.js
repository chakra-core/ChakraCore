'use strict';
var uniqueRandom = require('unique-random');

module.exports = function (arr) {
	var rand = uniqueRandom(0, arr.length - 1);

	return function () {
		return arr[rand()];
	};
};
