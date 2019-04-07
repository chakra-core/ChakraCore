'use strict';
const dotProp = require('dot-prop');
const arrify = require('arrify');

const dotPropGet = dotProp.get;

module.exports = (arr, prop) => {
	if (!Array.isArray(arr)) {
		throw new TypeError('Expected an array');
	}

	return arr.slice().sort((a, b) => {
		let ret = 0;

		arrify(prop).some(el => {
			let x;
			let y;
			let desc;

			if (typeof el === 'function') {
				x = el(a);
				y = el(b);
			} else if (typeof el === 'string') {
				desc = el.charAt(0) === '-';
				el = desc ? el.slice(1) : el;
				x = dotPropGet(a, el);
				y = dotPropGet(b, el);
			} else {
				x = a;
				y = b;
			}

			if (x === y) {
				ret = 0;
				return false;
			}

			if (y !== 0 && !y) {
				ret = desc ? 1 : -1;
				return true;
			}

			if (x !== 0 && !x) {
				ret = desc ? -1 : 1;
				return true;
			}

			if (typeof x === 'string' && typeof y === 'string') {
				ret = desc ? y.localeCompare(x) : x.localeCompare(y);
				return ret !== 0;
			}

			if (desc) {
				ret = x < y ? 1 : -1;
			} else {
				ret = x < y ? -1 : 1;
			}

			return true;
		});

		return ret;
	});
};
