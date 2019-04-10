'use strict';
var isRoot = require('is-root');
var defaultUid = require('default-uid');

module.exports = function () {
	if (isRoot()) {
		// setgid needs to happen before setuid to avoid EPERM
		if (process.setgid) {
			var gid = parseInt(process.env.SUDO_GID, 10);
			if (gid && gid > 0) {
				process.setgid(gid);
			}
		}
		if (process.setuid) {
			var uid = parseInt(process.env.SUDO_UID, 10) || defaultUid();
			if (uid && uid > 0) {
				process.setuid(uid);
			}
		}
	}
};
