'use strict';
module.exports = function (str) {
	if (typeof str !== 'string') {
		throw new TypeError('Expected a string');
	}

	return str.toLowerCase().replace(/(?:^|\s|-)\S/g, function (m) {
		return m.toUpperCase();
	});
};
