'use strict';
const pSome = require('p-some');

module.exports = (iterable, opts) => pSome(iterable, Object.assign({}, opts, {count: 1})).then(values => values[0]);

module.exports.AggregateError = pSome.AggregateError;
