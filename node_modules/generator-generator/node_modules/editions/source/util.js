'use strict'

// As Errlop uses Editions, we should use a specific Errlop edition
// As otherwise, the circular reference may fail on some machines
// https://github.com/bevry/errlop/issues/2
const Errlop = require('errlop/edition-node-0.8/index.js')

/**
 * Allow code and level inputs on Errlop.
 * We do this instead of a class extension, as class extensions do not interop well on node 0.8, which is our target.
 * @param {Object} opts
 * @param {string} opts.message
 * @param {string} opts.code
 * @param {string} opts.level
 * @param {Errlop|Error} [parent]
 * @returns {Errlop}
 * @private
 */
function errtion({ message, code, level }, parent) {
	if (this) throw new Error('errtion is not to be created with new')
	const error = new Errlop(message, parent)
	if (code) error.code = code
	if (level) error.level = level
	return error
}

/**
 * Converts anything to a string, by returning strings and serialising objects.
 * @param {any} value
 * @returns {string}
 * @private
 */
function stringify(value) {
	return typeof value === 'string' ? value : JSON.stringify(value)
}

/**
 * Converts a version range like `4 || 6` to `>=4`.
 * @param {string} range
 * @returns {string}
 * @private
 */
function simplifyRange(range) {
	return range.replace(/^([.\-\w]+)(\s.+)?$/, '>=$1')
}

module.exports = { errtion, stringify, simplifyRange }
