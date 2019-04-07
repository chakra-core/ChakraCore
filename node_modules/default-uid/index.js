'use strict';

var DEFAULT_UIDS = {
	darwin: 501,
	freebsd: 1000,
	linux: 1000,
	sunos: 100
};

module.exports = function (platform) {
	return DEFAULT_UIDS[platform || process.platform];
};
