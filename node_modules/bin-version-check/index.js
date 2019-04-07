'use strict';
const semver = require('semver');
const binVersion = require('bin-version');
const semverTruncate = require('semver-truncate');

module.exports = (bin, semverRange, opts) => {
	if (typeof bin !== 'string' || typeof semverRange !== 'string') {
		return Promise.reject('`binary` and `semverRange` required');
	}

	if (!semver.validRange(semverRange)) {
		return Promise.reject(new Error('Invalid version range'));
	}

	return binVersion(bin, opts).then(binVersion => {
		if (!semver.satisfies(semverTruncate(binVersion, 'patch'), semverRange)) {
			const err = new Error(`${bin} ${binVersion} doesn't satisfy the version requirement of ${semverRange}`);
			err.name = 'InvalidBinVersion';
			throw err;
		}
	});
};
