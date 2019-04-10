'use strict';
var decamelize = require('decamelize');

module.exports = function (str) {
	if (typeof str !== 'string') {
		throw new TypeError('Expected a string');
	}

	str = decamelize(str);
	str = str.toLowerCase().replace(/[_-]+/g, ' ').replace(/\s{2,}/g, ' ').trim();
	str = str.charAt(0).toUpperCase() + str.slice(1);

	return str;
};
