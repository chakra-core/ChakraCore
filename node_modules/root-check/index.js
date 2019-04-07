'use strict';
var downgradeRoot = require('downgrade-root');
var sudoBlock = require('sudo-block');

module.exports = function () {
	try {
		downgradeRoot();
	} catch (err) {}

	sudoBlock.apply(null, arguments);
};
