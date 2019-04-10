/* eslint no-console:0 */
'use strict'

// Imports
const pathUtil = require('path')
const semver = require('semver')
const { errtion, stringify, simplifyRange } = require('./util.js')

// As Errlop uses Editions, we should use a specific Errlop edition
// As otherwise, the circular reference may fail on some machines
// https://github.com/bevry/errlop/issues/2
const Errlop = require('errlop/edition-node-0.8/index.js')

/**
 * The current node version that we are operating within.
 * It is compared in {@link requireEdition} against {@link Edition.engines}.
 * @type {string}
 * @private
 */
const NODE_VERSION = process.versions.node

/**
 * Set the environment variable `EDITIONS_VERBOSE` to output debugging information to stderr on how editions selected the edition it did.
 * Values of `yes` and `true` are supported.
 * @example env EDITIONS_VERBOSE=true node mypackage/index.js
 * @typedef {String} EDITIONS_VERBOSE
 */

/**
 * Whether or not {@link EDITIONS_VERBOSE} is enabled.
 * @type {bollean}
 * @private
 */
const VERBOSE =
	process.env.EDITIONS_VERBOSE === true ||
	process.env.EDITIONS_VERBOSE === 'yes' ||
	process.env.EDITIONS_VERBOSE === 'true' ||
	false

/**
 * Set the environment variable `EDITIONS_TAG_BLACKLIST` to the tags you wish to blacklist, and editions will skip editions that contain them.
 * For backwards compatibility `EDITIONS_SYNTAX_BLACKLIST` is also supported.
 * It is compared in {@link requireEdition} against {@link Edition.tags}.
 * The value of this is stored locally in the {@link BLACKLIST} cache.
 * @example env EDITIONS_TAG_BLACKLIST=esnext,typescript,coffeescript node mypackage/index.js
 * @typedef {String} EDITIONS_TAG_BLACKLIST
 */

/**
 * A list of the blacklisted tags.
 * Data imported from {@link EDITIONS_TAG_BLACKLIST}.
 * @type {Array<string>}
 * @private
 */
const BLACKLIST =
	(process.env.EDITIONS_TAG_BLACKLIST &&
		process.env.EDITIONS_TAG_BLACKLIST.split(/[, ]+/g)) ||
	(process.env.EDITIONS_SYNTAX_BLACKLIST &&
		process.env.EDITIONS_SYNTAX_BLACKLIST.split(/[, ]+/g))

/**
 * A mapping of blacklisted tags to their reasons.
 * Keys are the tags.
 * Values are the error instances that contain the reasoning for why/how that tag is/became blacklisted.
 * Data imported from {@link EDITIONS_TAG_BLACKLIST}.
 * @type {Object.<string, Error>}
 * @private
 */
const blacklist = {}

/**
 * Edition entries must conform to the following specification.
 * @typedef {Object} Edition
 * @property {string} description Use this property to decribe the edition in human readable terms. Such as what it does and who it is for. It is used to reference the edition in user facing reporting, such as error messages. E.g. `esnext source code with require for modules`.
 * @property {string} directory The location to where this directory is located. It should be a relative path from the `package.json` file. E.g. `source`.
 * @property {string} entry The default entry location for this edition, relative to the edition's directory. E.g. `index.js`.
 * @property {Array<string>} [tags] Any keywords you wish to associate to the edition. Useful for various ecosystem tooling, such as automatic ESNext lint configuration if the `esnext` tag is present in the source edition tags. Consumers also make use of this via {@link EDITIONS_TAG_BLACKLIST} for preventing loading editions that contain a blacklisted tag. Previously this field was named `syntaxes`. E.g. `["javascript", "esnext", "require"]`.
 * @property {false|Object.<string, string|boolean>} engines This field is used to specific which Node.js and Browser environments this edition supports. If `false` this edition does not support either. If `node` is a string, it should be a semver range of node.js versions that the edition targets. If `browsers` is a string, it should be a [browserlist](https://github.com/browserslist/browserslist) value of the specific browser values the edition targets. If `node` or `browsers` is true, it indicates that this edition is compatible with those environments.
 * @example
 * {
 *   "description": "esnext source code with require for modules",
 *   "directory": "source",
 *   "entry": "index.js",
 *   "tags": [
 *     "javascript",
 *     "esnext",
 *     "require"
 *   ],
 *   "engines": {
 *     "node": ">=6",
 *     "browsers": "defaults"
 *   }
 * }
 */

/**
 * These are the various options that you can use to customise the behaviour of certain methods.
 * @typedef {Object} Options
 * @property {Function} require The require method of the calling module, used to ensure require paths remain correct.
 * @property {string} [packagePath] If provided, this is used for debugging.
 * @property {boolean} [verbose] If provided, any error loading an edition will be logged. By default, errors are only logged if all editions failed. If not provided, process.env.EDITIONS_VERBOSE is used.
 * @property {boolean} [strict] If `true`, then only exact version matches will be loaded. If `false`, then likely matches using {@link simplifyRange} will be evaluated, with a fallback to the last. If missing, then `true` is attempted first and if no result, then `false` is attempted.
 * @property {string} [cwd] If provided, this will be the cwd for entries.
 * @property {string} [entry] If provided, should be a relative path to the entry point of the edition.
 * @property {string} [package] If provided, should be the name of the package that we are loading the editions for.
 * @property {stream.Writable} [stderr] If not provided, will use process.stderr instead. It is the stream that verbose errors are logged to.
 */

// Create the mapping of blacklisted tags and their reasonings
if (BLACKLIST) {
	for (let i = 0; i < BLACKLIST.length; ++i) {
		const tag = BLACKLIST[i].trim().toLowerCase()
		blacklist[tag] = errtion({
			message: `The EDITIONS_TAG_BLACKLIST (aka EDITIONS_SYNTAX_BLACKLIST) environment variable blacklisted the tag [${tag}]`,
			code: 'blacklisted-tag'
		})
	}
}

// Blacklist the tag 'esnext' if our node version is below 0.12
if (semver.satisfies(NODE_VERSION, '<0.12')) {
	blacklist.esnext = new Error(
		'The esnext tag is skipped on early node versions as attempting to use esnext features will output debugging information on these node versions'
	)
}

/**
 * Attempt to load a specific {@link Edition}.
 * @param {Edition} edition
 * @param {Options} opts
 * @returns {*} The result of the loaded edition.
 * @throws {Error} An error if the edition failed to load.
 * @public
 */
function loadEdition(edition, opts) {
	const entry = pathUtil.resolve(
		opts.cwd || '',
		edition.directory,
		opts.entry || edition.entry
	)
	if (opts.require == null) {
		throw errtion({
			message: `Skipped edition [${
				edition.description
			}] as opts.require was not provided, this is probably due to a testing misconfiguration.`,
			code: 'unsupported-edition-require'
		})
	}
	try {
		return opts.require(entry)
	} catch (loadError) {
		// Note the error with more details
		throw errtion(
			{
				message: `Skipped edition [${
					edition.description
				}] at entry [${entry}] because it failed to load`,
				code: 'unsupported-edition-tried'
			},
			loadError
		)
	}
}

/**
 * Attempt to require an {@link Edition}, based on its compatibility with the current environment, such as {@link NODE_VERSION} and {@link EDITIONS_TAG_BLACKLIST} compatibility.
 * If compatibility is established with the environment, it will load the edition using {@link loadEdition}.
 * @param {Edition} edition
 * @param {Options} opts
 * @returns {*} the result of the loaded edition
 * @throws {Error} an error if the edition failed to load
 * @public
 */
function requireEdition(edition, opts) {
	// Verify the edition is valid
	if (
		!edition.description ||
		!edition.directory ||
		!edition.entry ||
		edition.engines == null
	) {
		throw errtion({
			message: `Each edition must have its [description, directory, entry, engines] fields defined, yet all it had was [${Object.keys(
				edition
			).join(', ')}]`,
			code: 'unsupported-edition-malformed',
			level: 'fatal'
		})
	}

	// Handle strict omission
	if (opts.strict == null) {
		try {
			return requireEdition(edition, { ...opts, strict: true })
		} catch (err) {
			return requireEdition(edition, { ...opts, strict: false })
		}
	}

	// Verify tag support
	// Convert tags into a sorted lowercase string
	const tags = (edition.tags || edition.syntaxes || [])
		.map(i => i.toLowerCase())
		.sort()
	for (let index = 0; index < tags.length; index++) {
		const tag = tags[index]
		const blacklisted = blacklist[tag]
		if (blacklisted) {
			throw errtion(
				{
					message: `Skipping edition [${
						edition.description
					}] because it contained a blacklisted tag [${tag}]`,
					code: 'unsupported-edition-backlisted-tag'
				},
				blacklisted
			)
		}
	}

	// Verify engine support
	if (edition.engines === false) {
		throw errtion({
			message: `Skipping edition [${
				edition.description
			}] because its engines field was false`,
			code: 'unsupported-edition-engine'
		})
	}
	if (!edition.engines.node) {
		throw errtion({
			message: `Skipping edition [${
				edition.description
			}] because its .engines.node field was falsey`,
			code: 'unsupported-edition-engines-node'
		})
	}
	if (opts.strict) {
		if (edition.engines.node === true) {
			throw errtion({
				message: `Skipping edition [${
					edition.description
				}] because its .engines.node field was true yet we are in strict mode`,
				code: 'unsupported-edition-engines-node-version-true'
			})
		} else if (semver.satisfies(NODE_VERSION, edition.engines.node) === false) {
			throw errtion({
				message: `Skipping edition [${
					edition.description
				}] because our current node version [${NODE_VERSION}] is not supported by its specific range [${stringify(
					edition.engines.node
				)}]`,
				code: 'unsupported-edition-engines-node-version-specific'
			})
		}
	} else if (edition.engines.node !== true) {
		const simplifiedRange = simplifyRange(edition.engines.node)
		if (semver.satisfies(NODE_VERSION, simplifiedRange) === false) {
			throw errtion({
				message: `Skipping edition [${
					edition.description
				}] because our current node version [${NODE_VERSION}] is not supported by its simplified range [${stringify(
					simplifiedRange
				)}]`,
				code: 'unsupported-edition-engines-node-version-simplified'
			})
		}
	}

	// Load the edition
	return loadEdition(edition, opts)
}

/**
 * Cycles through a list of editions, returning the require result of the first suitable {@link Edition} that it was able to load.
 * Editions should be ordered from most preferable first, to least desirable last.
 * Providing the editions configuration is valid, individual edition handling is forwarded to {@link requireEdition}.
 * @param {Array<Edition>} editions
 * @param {Options} opts
 * @returns {*} The result of the loaded edition.
 * @throws {Error} An error if a suitable edition was unable to be resolved.
 * @public
 */
function requireEditions(editions, opts) {
	// Check
	if (!editions || editions.length === 0) {
		if (opts.packagePath) {
			throw errtion({
				message: `There were no editions specified for package [${
					opts.packagePath
				}]`,
				code: 'unsupported-editions-missing'
			})
		} else {
			throw errtion({
				message: 'There were no editions specified',
				code: 'unsupported-editions-missing'
			})
		}
	}

	// Handle strict omission
	if (opts.strict == null) {
		try {
			return requireEditions(editions, { ...opts, strict: true })
		} catch (err) {
			return requireEditions(editions, { ...opts, strict: false })
		}
	}

	// Whether or not we should be verbose
	const verbose = opts.verbose == null ? VERBOSE : opts.verbose

	// Capture the load result, the last error, and the fallback option
	let result,
		loaded = false,
		editionsError = null,
		fallbackEdition = null

	// Cycle through the editions determing the above
	for (let i = 0; i < editions.length; ++i) {
		const edition = editions[i]
		try {
			result = requireEdition(edition, opts)
			loaded = true
			break
		} catch (editionError) {
			if (editionError.level === 'fatal') {
				editionsError = editionError
				break
			} else if (editionsError) {
				editionsError = errtion(editionsError, editionError)
			} else {
				editionsError = editionError
			}
			// make the fallback edition one that we don't bother loading due to its engines
			// also: don't assume that .code is accessible, as it may not be, even if it should be, due to the way different environments behave
			if (
				String(editionError.code || '').indexOf(
					'unsupported-edition-engines-node-version'
				) === 0
			) {
				fallbackEdition = edition
			}
		}
	}

	// if no edition was suitable for our environment, then try the fallback if it exists
	// that is to say, ignore its engines.node
	if (opts.strict === false && loaded === false && fallbackEdition) {
		try {
			result = loadEdition(fallbackEdition, opts)
			loaded = true
		} catch (editionError) {
			editionsError = new Errlop(editionError, editionsError)
		}
	}

	// if we were able to load something, then provide it
	if (loaded) {
		// make note of any errors if desired
		if (editionsError && verbose) {
			const stderr = opts.stderr || process.stderr
			stderr.write(editionsError.stack + '\n')
		}
		return result
	}
	// otherwise, provide the error
	else if (editionsError) {
		if (opts.packagePath) {
			throw errtion(
				{
					message: `There were no suitable editions for package [${
						opts.packagePath
					}]`,
					code: 'unsupported-editions-tried'
				},
				editionsError
			)
		} else {
			throw errtion(
				{
					message: 'There were no suitable editions',
					code: 'unsupported-editions-tried'
				},
				editionsError
			)
		}
	}
}

/**
 * Cycle through the editions for a package and require the correct one.
 * Providing the package configuration is valid, editions handling is forwarded to {@link requireEditions}.
 * @param {Options.cwd} cwd
 * @param {Options.require} require
 * @param {Options.entry} [entry]
 * @returns {any} The result of the loaded edition.
 * @throws {Error} An error if a suitable edition was unable to be resolved.
 */
function requirePackage(cwd, require, entry) {
	// Load the package.json file to fetch `name` for debugging and `editions` for loading
	const packagePath = pathUtil.resolve(cwd, 'package.json')
	const { editions } = require(packagePath)
	const opts = { packagePath, cwd, require, entry }
	return requireEditions(editions, opts)
}

// Exports
module.exports = { requireEdition, requireEditions, requirePackage }
