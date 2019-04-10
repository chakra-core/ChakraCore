'use strict';
const got = require('got');
const isScoped = require('is-scoped');
const registryUrl = require('registry-url')();
const zip = require('lodash.zip');

function request(name) {
	if (isScoped(name)) {
		name = name.replace(/\//g, '%2f');
	}

	return got.head(registryUrl + name.toLowerCase(), {timeout: 10000})
		.then(() => false)
		.catch(err => {
			if (err.statusCode === 404) {
				return true;
			}

			throw err;
		});
}

module.exports = name => {
	if (!(typeof name === 'string' && name.length !== 0)) {
		return Promise.reject(new Error('Package name required'));
	}

	return request(name);
};

module.exports.many = names => {
	if (!Array.isArray(names)) {
		return Promise.reject(new TypeError(`Expected an array, got ${typeof names}`));
	}

	return Promise.all(names.map(request))
		.then(result => new Map(zip(names, result)));
};
