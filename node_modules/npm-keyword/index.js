'use strict';
const got = require('got');
const registryUrl = require('registry-url');

function get(keyword, options) {
	if (typeof keyword !== 'string' && !Array.isArray(keyword)) {
		return Promise.reject(new TypeError('Keyword must be either a string or an array of strings'));
	}

	if (options.size < 1 || options.size > 250) {
		return Promise.reject(new TypeError('Size option must be between 1 and 250'));
	}

	keyword = encodeURIComponent(keyword).replace('%2C', '+');

	const url = `${registryUrl()}-/v1/search?text=keywords:${keyword}&size=${options.size}`;

	return got(url, {json: true}).then(response => response.body);
}

module.exports = (keyword, options) => {
	options = Object.assign({size: 250}, options);

	return get(keyword, options).then(data => {
		return data.objects.map(el => ({
			name: el.package.name,
			description: el.package.description
		}));
	});
};

module.exports.names = (keyword, options) => {
	options = Object.assign({size: 250}, options);

	return get(keyword, options).then(data => data.objects.map(x => x.package.name));
};

module.exports.count = keyword => {
	return get(keyword, {size: 1}).then(data => data.total);
};
