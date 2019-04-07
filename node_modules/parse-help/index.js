'use strict';
const execall = require('execall');

module.exports = input => {
	const ret = {
		flags: {},
		aliases: {}
	};

	const regex = /^\s*[^$]\s*(?:-([a-z-]),[ \t]+)?--([a-z-]+) +(.*)$/igm;

	for (const match of execall(regex, input)) {
		const submatch = match.sub;

		if (submatch[0]) {
			ret.aliases[submatch[0]] = submatch[1];
		}

		if (submatch[1]) {
			ret.flags[submatch[1]] = {};

			const flag = ret.flags[submatch[1]];

			if (submatch[0]) {
				flag.alias = submatch[0];
			}

			if (submatch[2]) {
				flag.description = submatch[2];
			}
		}
	}

	return ret;
};
