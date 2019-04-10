# Generate Password [![Build Status](https://travis-ci.org/brendanashworth/generate-password.svg?branch=master)](https://travis-ci.org/brendanashworth/generate-password) [![codecov](https://codecov.io/gh/brendanashworth/generate-password/branch/master/graph/badge.svg)](https://codecov.io/gh/brendanashworth/generate-password)

[![Generate-Password NPM](https://nodei.co/npm/generate-password.png?downloads=true&downloadRank=true)](http://npmjs.org/package/generate-password)

> Generate Password is a (relatively) extensive library for generating random and unique passwords.

## Install

```bash
$ npm install generate-password --save
```

## Usage

#### `generate([options])`

Generate one password with the given options. Returns a string.

```javascript
var generator = require('generate-password');

var password = generator.generate({
	length: 10,
	numbers: true
});

// 'uEyMTw32v9'
console.log(password);
```

#### `generateMultiple(amount[, options])`

Bulk generate multiple passwords at once, with the same options for all. Returns an array.

```javascript
var generator = require('generate-password');

var passwords = generator.generateMultiple(3, {
	length: 10,
	uppercase: false
});

// [ 'hnwulsekqn', 'qlioullgew', 'kosxwabgjv' ]
console.log(passwords);
```

### Available options
Any of these can be passed into the options object for each function.


|            Name          |                  Description                        | Default Value |
|--------------------------|-----------------------------------------------------|---------------|
| length                   | Integer, length of password.                        |       10      |
| numbers                  | Boolean, put numbers in password.                   |     false     |
| symbols                  | Boolean, put symbols in password.                   |     false     |
| uppercase                | Boolean, use uppercase letters in password.         |      true     |
| excludeSimilarCharacters | Boolean, exclude similar chars, like 'i' and 'l'.   |     false     |
| exclude                  | String, characters to be excluded from password.    |       ''      |
| strict                   | Boolean, password must include at least one character from each pool. |     false     |
