'use strict';
const fs = require('fs');
const execa = require('execa');
const pify = require('pify');

function extractDarwin(line) {
	const cols = line.split(':');

	// Darwin passwd(5)
	// 0 name      User's login name.
	// 1 password  User's encrypted password.
	// 2 uid       User's id.
	// 3 gid       User's login group id.
	// 4 class     User's general classification (unused).
	// 5 change    Password change time.
	// 6 expire    Account expiration time.
	// 7 gecos     User's full name.
	// 8 home_dir  User's home directory.
	// 9 shell     User's login shell.

	return {
		username: cols[0],
		password: cols[1],
		uid: Number(cols[2]),
		gid: Number(cols[3]),
		fullname: cols[7],
		homedir: cols[8],
		shell: cols[9]
	};
}

function extractLinux(line) {
	const cols = line.split(':');

	// Linux passwd(5):
	// 0 login name
	// 1 optional encrypted password
	// 2 numerical user ID
	// 3 numerical group ID
	// 4 user name or comment field
	// 5 user home directory
	// 6 optional user command interpreter

	return {
		username: cols[0],
		password: cols[1],
		uid: Number(cols[2]),
		gid: Number(cols[3]),
		fullname: cols[4] && cols[4].split(',')[0],
		homedir: cols[5],
		shell: cols[6]
	};
}

function getUser(str, username) {
	const extract = process.platform === 'linux' ? extractLinux : extractDarwin;
	const lines = str.split('\n');
	const l = lines.length;
	let i = 0;

	while (i < l) {
		const user = extract(lines[i++]);

		if (user.username === username || user.uid === Number(username)) {
			return user;
		}
	}
}

module.exports = username => {
	if (username === undefined) {
		if (typeof process.getuid !== 'function') {
			return Promise.reject(new Error('Platform not supported'));
		}

		username = process.getuid();
	}

	if (process.platform === 'linux') {
		return pify(fs.readFile)('/etc/passwd', 'utf8')
			.then(passwd => getUser(passwd, username));
	}

	if (process.platform === 'darwin') {
		return execa('/usr/bin/id', ['-P', username])
			.then(x => getUser(x.stdout, username));
	}

	return Promise.reject(new Error('Platform not supported'));
};

module.exports.sync = username => {
	if (username === undefined) {
		if (typeof process.getuid !== 'function') {
			throw new Error('Platform not supported');
		}

		username = process.getuid();
	}

	if (process.platform === 'linux') {
		return getUser(fs.readFileSync('/etc/passwd', 'utf8'), username);
	}

	if (process.platform === 'darwin') {
		return getUser(execa.sync('/usr/bin/id', ['-P', username]).stdout, username);
	}

	throw new Error('Platform not supported');
};
