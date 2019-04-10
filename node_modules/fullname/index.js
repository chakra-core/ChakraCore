'use strict';

const mem = require('mem');
const execa = require('execa');
const passwdUser = require('passwd-user');
const pAny = require('p-any');
const pTry = require('p-try');
const filterObj = require('filter-obj');

const envVars = [
	'GIT_AUTHOR_NAME',
	'GIT_COMMITTER_NAME',
	'HGUSER', // Mercurial
	'C9_USER' // Cloud9
];

function checkEnv() {
	return pTry(() => {
		const env = process.env;

		const varName = envVars.find(x => env[x]);
		const fullname = varName && env[varName];

		return fullname || Promise.reject();
	});
}

function checkAuthorName() {
	return pTry(() => {
		const fullname = require('rc')('npm')['init-author-name'];
		return fullname || Promise.reject();
	});
}

function checkPasswd() {
	return passwdUser()
		.then(user => user.fullname || Promise.reject());
}

function checkGit() {
	return execa.stdout('git', ['config', '--global', 'user.name'])
		.then(fullname => fullname || Promise.reject());
}

function checkOsaScript() {
	return execa.stdout('osascript', ['-e', 'long user name of (system info)'])
		.then(fullname => fullname || Promise.reject());
}

function checkWmic() {
	return execa.stdout('wmic', ['useraccount', 'where', 'name="%username%"', 'get', 'fullname'])
		.then(stdout => {
			const fullname = stdout.replace('FullName', '');
			return fullname || Promise.reject();
		});
}

function checkGetEnt() {
	return execa.shell('getent passwd $(whoami)').then(res => {
		const fullname = (res.stdout.split(':')[4] || '').replace(/,.*/, '');
		return fullname || Promise.reject();
	});
}

function fallback() {
	if (process.platform === 'darwin') {
		return pAny([checkPasswd(), checkOsaScript()]);
	}

	if (process.platform === 'win32') {
		// Fullname is usually not set by default in the system on Windows 7+
		return pAny([checkGit(), checkWmic()]);
	}

	return pAny([checkPasswd(), checkGetEnt(), checkGit()]);
}

function getFullName() {
	return checkEnv()
		.catch(checkAuthorName)
		.catch(fallback)
		.catch(() => {});
}

module.exports = mem(getFullName, {
	cacheKey() {
		return JSON.stringify(filterObj(process.env, envVars));
	}
});
