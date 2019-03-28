var crypto = require('crypto');

var self = module.exports;

// Generates a random number
var randomNumber = function(max) {
	// gives a number between 0 (inclusive) and max (exclusive)
	var rand = crypto.randomBytes(1)[0];
	while (rand >= 256 - (256 % max)) {
		rand = crypto.randomBytes(1)[0];
	}
	return rand % max;
};

// Possible combinations
var lowercase = 'abcdefghijklmnopqrstuvwxyz',
	uppercase = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ',
	numbers = '0123456789',
	symbols = '!@#$%^&*()+_-=}{[]|:;"/?.><,`~',
	similarCharacters = /[ilLI|`oO0]/g,
	strictRules = [
		{ name: 'lowercase', rule: /[a-z]/ },
		{ name: 'uppercase', rule: /[A-Z]/ },
		{ name: 'numbers', rule: /[0-9]/ },
		{ name: 'symbols', rule: /[!@#$%^&*()+_\-=}{[\]|:;"/?.><,`~]/ }
	];

var generate = function(options, pool) {
	var password = '',
		optionsLength = options.length,
		poolLength = pool.length;

	for (var i = 0; i < optionsLength; i++) {
		password += pool[randomNumber(poolLength)];
	}

	if (options.strict) {
		// Iterate over each rule, checking to see if the password works.
		var fitsRules = strictRules.reduce(function(result, rule) {
			// Skip checking the rule if we know it doesn't match.
			if (result == false) return false;

			// If the option is not checked, ignore it.
			if (options[rule.name] == false) return result;

			// Run the regex on the password and return whether
			// or not it matches.
			return rule.rule.test(password);
		}, true);

		// If it doesn't fit the rules, generate a new one (recursion).
		if (!fitsRules) return generate(options, pool);
	}

	return password;
};

// Generate a random password.
self.generate = function(options) {
	// Set defaults.
	options = options || {};
	if (!options.hasOwnProperty('length')) options.length = 10;
	if (!options.hasOwnProperty('numbers')) options.numbers = false;
	if (!options.hasOwnProperty('symbols')) options.symbols = false;
	if (!options.hasOwnProperty('exclude')) options.exclude = '';
	if (!options.hasOwnProperty('uppercase')) options.uppercase = true;
	if (!options.hasOwnProperty('excludeSimilarCharacters')) options.excludeSimilarCharacters = false;
	if (!options.hasOwnProperty('strict')) options.strict = false;

	if (options.strict) {
		var minStrictLength = 1 + (options.numbers ? 1 : 0) + (options.symbols ? 1 : 0) + (options.uppercase ? 1 : 0);
		if (minStrictLength > options.length) {
			throw new TypeError('Length must correlate with strict guidelines');
		}
	}

	// Generate character pool
	var pool = lowercase;

	// uppercase
	if (options.uppercase) {
		pool += uppercase;
	}
	// numbers
	if (options.numbers) {
		pool += numbers;
	}
	// symbols
	if (options.symbols) {
		pool += symbols;
	}

	// similar characters
	if (options.excludeSimilarCharacters) {
		pool = pool.replace(similarCharacters, '');
	}

	// excludes characters from the pool
	var i = options.exclude.length;
	while (i--) {
		pool = pool.replace(options.exclude[i], '');
	}

	var password = generate(options, pool);

	return password;
};

// Generates multiple passwords at once with the same options.
self.generateMultiple = function(amount, options) {
	var passwords = [];

	for (var i = 0; i < amount; i++) {
		passwords[i] = self.generate(options);
	}

	return passwords;
};
