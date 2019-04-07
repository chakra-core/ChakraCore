'use strict';
var childProcess = require('child_process');
var crossSpawnAsync = require('cross-spawn-async');
var stripEof = require('strip-eof');
var objectAssign = require('object-assign');
var TEN_MEBIBYTE = 1024 * 1024 * 10;

module.exports = function (cmd, args, opts) {
	return new Promise(function (resolve, reject) {
		opts = objectAssign({
			maxBuffer: TEN_MEBIBYTE
		}, opts);

		var parsed = crossSpawnAsync._parse(cmd, args, opts);

		var handle = function (val) {
			if (parsed.options.stripEof !== false) {
				val = stripEof(val);
			}

			return val;
		};

		var spawned = childProcess.execFile(parsed.command, parsed.args, parsed.options, function (err, stdout, stderr) {
			if (err) {
				reject(err);
				return;
			}

			resolve({
				stdout: handle(stdout),
				stderr: handle(stderr)
			});
		});

		crossSpawnAsync._enoent.hookChildProcess(spawned, parsed);
	});
};

module.exports.shell = function (cmd, opts) {
	var file;
	var args;

	opts = objectAssign({}, opts);

	if (process.platform === 'win32') {
		file = process.env.comspec || 'cmd.exe';
		args = ['/s', '/c', '"' + cmd + '"'];
		opts.windowsVerbatimArguments = true;
	} else {
		file = '/bin/sh';
		args = ['-c', cmd];
	}

	if (opts.shell) {
		file = opts.shell;
	}

	return module.exports(file, args, opts);
};
