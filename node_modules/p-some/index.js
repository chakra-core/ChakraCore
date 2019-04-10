'use strict';
const AggregateError = require('aggregate-error');

module.exports = (iterable, opts) => new Promise((resolve, reject) => {
	opts = Object.assign({}, opts);

	if (!Number.isFinite(opts.count)) {
		throw new TypeError(`Expected a finite number, got ${typeof opts.count}`);
	}

	const values = [];
	const errors = [];
	let elCount = 0;
	let maxErrors = -opts.count + 1;
	let maxFiltered = -opts.count + 1;
	let done = false;

	const fulfilled = value => {
		if (done) {
			return;
		}

		if (typeof opts.filter === 'function' && !opts.filter(value)) {
			if (--maxFiltered === 0) {
				done = true;
				reject(new RangeError(`Not enough values pass the \`filter\` option`));
			}

			return;
		}

		values.push(value);

		if (--opts.count === 0) {
			done = true;
			resolve(values);
		}
	};

	const rejected = error => {
		if (done) {
			return;
		}

		errors.push(error);

		if (--maxErrors === 0) {
			done = true;
			reject(new AggregateError(errors));
		}
	};

	for (const el of iterable) {
		maxErrors++;
		maxFiltered++;
		elCount++;
		Promise.resolve(el).then(fulfilled, rejected);
	}

	if (opts.count > elCount) {
		throw new RangeError(`Expected input to contain at least ${opts.count} items, but contains ${elCount} items`);
	}
});

module.exports.AggregateError = AggregateError;
