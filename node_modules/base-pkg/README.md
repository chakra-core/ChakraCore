# base-pkg [![NPM version](https://img.shields.io/npm/v/base-pkg.svg?style=flat)](https://www.npmjs.com/package/base-pkg) [![NPM monthly downloads](https://img.shields.io/npm/dm/base-pkg.svg?style=flat)](https://npmjs.org/package/base-pkg) [![NPM total downloads](https://img.shields.io/npm/dt/base-pkg.svg?style=flat)](https://npmjs.org/package/base-pkg) [![Linux Build Status](https://img.shields.io/travis/node-base/base-pkg.svg?style=flat&label=Travis)](https://travis-ci.org/node-base/base-pkg)

> Plugin for adding a `pkg` method that exposes pkg-store to your base application.

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install --save base-pkg
```

## Usage

```js
var pkg = require('base-pkg');
var Base = require('base');
var app = new Base();

app.use(pkg());

console.log(app.pkg.data);
//=> {"name": "my-project", ...}
```

## API

Visit [pkg-store](https://github.com/jonschlinkert/pkg-store) for additional API details and documentation.

```js
var pkg = require('base-pkg');
var Base = require('base');
var app = new Base();
```

### .pkg.set

```js
app.pkg.set(key, value);
```

Set property `key` with the given `value`.

**Example**

```js
// given {"name": "my-project"}
app.pkg.set('bin.foo', 'bar');

console.log(app.pkg.data);
//=> {"name": "my-project", "bin": {"foo": "bar"}}
```

### .pkg.save

Persist package.json to the file system at `app.pkg.path`.

```js
app.pkg.save();
```

### .pkg.get

```js
app.pkg.get(key);
```

Get property `key` from package.json.

**Example**

```js
// given {"name": "my-project"}
app.pkg.set('bin.foo', 'bar');

console.log(app.pkg.get('bin'));
//=> {"foo": "bar"}
```

### .pkg.has

```js
app.pkg.has(key);
```

Returns `true` if `package.json` has property `key`.

**Example**

```js
// given: {"name": "my-project"}
console.log(app.pkg.has('name'));
//=> true
console.log(app.pkg.has('zzzzzzz'));
//=> false
```

### .pkg.union

```js
app.pkg.union(key, val);
```

Create array `key`, or concatenate values to array `key`. Also uniquifies the array.

**Example**

```js
app.pkg.union('keywords', 'foo');
app.pkg.union('keywords', ['bar', 'baz']);

console.log(app.pkg.get('keywords'));
//=> ['foo', 'bar', 'baz']
```

## .pkg.expand

Creates a get/set API using [cache-base](https://github.com/jonschlinkert/cache-base), where the cache is populated with a shallow clone of `package.json` with values expanded by [expand-pkg](https://github.com/jonschlinkert/expand-pkg).

**Example**

```js
console.log(app.pkg.get('author'));
//=> 'Jon Schlinkert (https://github.com/jonschlinkert)'

var expanded = app.pkg.expand();
var author = expanded.get('author');
//=> {name: 'Jon Schlinkert', url: 'https://github.com/jonschlinkert'}
```

## Logging methods

A handful of logging are exposed, as a convenience for implementors to log updates to package.json with visual consistency.

### .pkg.logValue

Log a value.

```js
app.pkg.logValue('current value is:', {reflinks: ['foo', 'bar']});
```

**Example**

![logValue example](assets/log-value.jpg)

### .pkg.logInfo

Log an info message.

```js
app.pkg.logInfo('udpated value:', {reflinks: ['foo', 'bar']});
```

**Example**

![logInfo example](assets/log-info.jpg)

### .pkg.logWarning

Log an warning message.

```js
app.pkg.logWarning('deleted value:', {reflinks: ['foo', 'bar']});
```

**Example**

![logWarning example](assets/log-warning.jpg)

### .pkg.logSuccess

Log a success message.

```js
app.pkg.logSuccess('added value:', {reflinks: ['foo', 'bar']});
```

**Example**

![logSuccess example](assets/log-success.jpg)

### .pkg.logError

Log an error message.

```js
app.pkg.logError('missing value:', {reflinks: ['foo', 'bar']});
```

**Example**

![logError example](assets/log-error.jpg)

## About

### Related projects

* [base-options](https://www.npmjs.com/package/base-options): Adds a few options methods to base-methods, like `option`, `enable` and `disable`. See the readme… [more](https://github.com/jonschlinkert/base-options) | [homepage](https://github.com/jonschlinkert/base-options "Adds a few options methods to base-methods, like `option`, `enable` and `disable`. See the readme for the full API.")
* [base](https://www.npmjs.com/package/base): Framework for rapidly creating high quality node.js applications, using plugins like building blocks | [homepage](https://github.com/node-base/base "Framework for rapidly creating high quality node.js applications, using plugins like building blocks")
* [cache-base](https://www.npmjs.com/package/cache-base): Basic object cache with `get`, `set`, `del`, and `has` methods for node.js/javascript projects. | [homepage](https://github.com/jonschlinkert/cache-base "Basic object cache with `get`, `set`, `del`, and `has` methods for node.js/javascript projects.")
* [pkg-store](https://www.npmjs.com/package/pkg-store): Use package.json as a config store. | [homepage](https://github.com/jonschlinkert/pkg-store "Use package.json as a config store.")

### Contributing

Pull requests and stars are always welcome. For bugs and feature requests, [please create an issue](../../issues/new).

### Building docs

_(This project's readme.md is generated by [verb](https://github.com/verbose/verb-generate-readme), please don't edit the readme directly. Any changes to the readme must be made in the [.verb.md](.verb.md) readme template.)_

To generate the readme, run the following command:

```sh
$ npm install -g verbose/verb#dev verb-generate-readme && verb
```

### Running tests

Running and reviewing unit tests is a great way to get familiarized with a library and its API. You can install dependencies and run tests with the following command:

```sh
$ npm install && npm test
```

### Author

**Jon Schlinkert**

* [github/jonschlinkert](https://github.com/jonschlinkert)
* [twitter/jonschlinkert](https://twitter.com/jonschlinkert)

### License

Copyright © 2017, [Jon Schlinkert](https://github.com/jonschlinkert).
Released under the [MIT License](LICENSE).

***

_This file was generated by [verb-generate-readme](https://github.com/verbose/verb-generate-readme), v0.6.0, on July 16, 2017._