/*eslint no-unused-vars: "off"*/
var generator = require('./main');

// Generate one password.
var password = generator.generate({
	length: 15, // defaults to 10
	numbers: true, // defaults to false
	symbols: true, // defaults to false
	uppercase: true, // defaults to true
	strict: true // defaults to false
});

// Generate ten bulk.
var passwords = generator.generateMultiple(10, {
	length: 15,
	numbers: true,
	symbols: true,
	uppercase: true
});
