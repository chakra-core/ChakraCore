'use strict';
module.exports = function (min, max) {
	var prev;
	return function rand() {
		var num = Math.floor(Math.random() * (max - min + 1) + min);
		return prev = num === prev && min !== max ? rand() : num;
	};
};
