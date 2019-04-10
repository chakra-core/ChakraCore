'use strict';

const readFile = require('graceful-fs').readFile;

module.exports = function fsReadFilePromise(filePath, options) {
	return new Promise((resolve, reject) => {
		readFile(filePath, options, (err, data) => {
			if (err) {
				reject(err);
				return;
			}

			resolve(data);
		});
	});
};
