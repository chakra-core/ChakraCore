'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const assert = require('assert');
const EventEmitter = require('events');
const dotProp = require('dot-prop');
const makeDir = require('make-dir');
const pkgUp = require('pkg-up');
const envPaths = require('env-paths');
const writeFileAtomic = require('write-file-atomic');

const obj = () => Object.create(null);

// Prevent caching of this module so module.parent is always accurate
delete require.cache[__filename];
const parentDir = path.dirname((module.parent && module.parent.filename) || '.');

class Conf {
	constructor(opts) {
		const pkgPath = pkgUp.sync(parentDir);

		opts = Object.assign({
			// Can't use `require` because of Webpack being annoying:
			// https://github.com/webpack/webpack/issues/196
			projectName: pkgPath && JSON.parse(fs.readFileSync(pkgPath, 'utf8')).name
		}, opts);

		if (!opts.projectName && !opts.cwd) {
			throw new Error('Project name could not be inferred. Please specify the `projectName` option.');
		}

		opts = Object.assign({
			configName: 'config'
		}, opts);

		if (!opts.cwd) {
			opts.cwd = envPaths(opts.projectName).config;
		}

		this.events = new EventEmitter();
		this.encryptionKey = opts.encryptionKey;
		this.path = path.resolve(opts.cwd, `${opts.configName}.json`);
		this.store = Object.assign(obj(), opts.defaults, this.store);
	}
	get(key, defaultValue) {
		return dotProp.get(this.store, key, defaultValue);
	}
	set(key, val) {
		if (typeof key !== 'string' && typeof key !== 'object') {
			throw new TypeError(`Expected \`key\` to be of type \`string\` or \`object\`, got ${typeof key}`);
		}

		const store = this.store;

		if (typeof key === 'object') {
			for (const k of Object.keys(key)) {
				dotProp.set(store, k, key[k]);
			}
		} else {
			dotProp.set(store, key, val);
		}

		this.store = store;
	}
	has(key) {
		return dotProp.has(this.store, key);
	}
	delete(key) {
		const store = this.store;
		dotProp.delete(store, key);
		this.store = store;
	}
	clear() {
		this.store = obj();
	}
	onDidChange(key, callback) {
		if (typeof key !== 'string') {
			throw new TypeError(`Expected \`key\` to be of type \`string\`, got ${typeof key}`);
		}

		if (typeof callback !== 'function') {
			throw new TypeError(`Expected \`callback\` to be of type \`function\`, got ${typeof callback}`);
		}

		let currentValue = this.get(key);

		const onChange = () => {
			const oldValue = currentValue;
			const newValue = this.get(key);

			try {
				assert.deepEqual(newValue, oldValue);
			} catch (err) {
				currentValue = newValue;
				callback.call(this, newValue, oldValue);
			}
		};

		this.events.on('change', onChange);
		return () => this.events.removeListener('change', onChange);
	}
	get size() {
		return Object.keys(this.store).length;
	}
	get store() {
		try {
			let data = fs.readFileSync(this.path, this.encryptionKey ? null : 'utf8');

			if (this.encryptionKey) {
				try {
					const decipher = crypto.createDecipher('aes-256-cbc', this.encryptionKey);
					data = Buffer.concat([decipher.update(data), decipher.final()]);
				} catch (err) {/* ignore */}
			}

			return Object.assign(obj(), JSON.parse(data));
		} catch (err) {
			if (err.code === 'ENOENT') {
				makeDir.sync(path.dirname(this.path));
				return obj();
			}

			if (err.name === 'SyntaxError') {
				return obj();
			}

			throw err;
		}
	}
	set store(val) {
		// Ensure the directory exists as it could have been deleted in the meantime
		makeDir.sync(path.dirname(this.path));

		let data = JSON.stringify(val, null, '\t');

		if (this.encryptionKey) {
			const cipher = crypto.createCipher('aes-256-cbc', this.encryptionKey);
			data = Buffer.concat([cipher.update(Buffer.from(data)), cipher.final()]);
		}

		writeFileAtomic.sync(this.path, data);
		this.events.emit('change');
	}
	// TODO: Use `Object.entries()` here at some point
	* [Symbol.iterator]() {
		const store = this.store;

		for (const key of Object.keys(store)) {
			yield [key, store[key]];
		}
	}
}

module.exports = Conf;
