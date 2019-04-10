'use strict';
const execa = require('execa');
const findVersions = require('find-versions');

module.exports = (bin, opts) => {
	opts = opts || {};

	return execa(bin, opts.args || ['--version'])
		.then(result => findVersions(result.stdout || result.stderr, {loose: true})[0])
		.catch(err => {
			if (err.code === 'ENOENT') {
				err.message = `Couldn't find the '${bin}' binary. Make sure it's installed and in your $PATH`;
			}

			throw err;
		});
};
