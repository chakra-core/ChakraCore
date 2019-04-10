# base-helpers [![NPM version](https://img.shields.io/npm/v/base-helpers.svg?style=flat)](https://www.npmjs.com/package/base-helpers) [![NPM monthly downloads](https://img.shields.io/npm/dm/base-helpers.svg?style=flat)](https://npmjs.org/package/base-helpers)  [![NPM total downloads](https://img.shields.io/npm/dt/base-helpers.svg?style=flat)](https://npmjs.org/package/base-helpers) [![Linux Build Status](https://img.shields.io/travis/node-base/base-helpers.svg?style=flat&label=Travis)](https://travis-ci.org/node-base/base-helpers)

> Adds support for managing template helpers to your base application.

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install --save base-helpers
```

## Usage

Register the plugin with your [base](https://github.com/node-base/base) application:

```js
var Base = require('base');
var helpers = require('base-helpers');
base.use(helpers());
```

## API

### [.helper](index.js#L46)

Register a template helper.

**Params**

* `name` **{String}**: Helper name
* `fn` **{Function}**: Helper function.

**Example**

```js
app.helper('upper', function(str) {
  return str.toUpperCase();
});
```

### [.helpers](index.js#L67)

Register multiple template helpers.

**Params**

* `helpers` **{Object|Array}**: Object, array of objects, or glob patterns.

**Example**

```js
app.helpers({
  foo: function() {},
  bar: function() {},
  baz: function() {}
});
```

### [.asyncHelper](index.js#L89)

Register an async helper.

**Params**

* `name` **{String}**: Helper name.
* `fn` **{Function}**: Helper function

**Example**

```js
app.asyncHelper('upper', function(str, next) {
  next(null, str.toUpperCase());
});
```

### [.asyncHelpers](index.js#L110)

Register multiple async template helpers.

**Params**

* `helpers` **{Object|Array}**: Object, array of objects, or glob patterns.

**Example**

```js
app.asyncHelpers({
  foo: function() {},
  bar: function() {},
  baz: function() {}
});
```

### [.getHelper](index.js#L130)

Get a previously registered helper.

**Params**

* `name` **{String}**: Helper name
* `returns` **{Function}**: Returns the registered helper function.

**Example**

```js
var fn = app.getHelper('foo');
```

### [.getAsyncHelper](index.js#L147)

Get a previously registered async helper.

**Params**

* `name` **{String}**: Helper name
* `returns` **{Function}**: Returns the registered helper function.

**Example**

```js
var fn = app.getAsyncHelper('foo');
```

### [.hasHelper](index.js#L166)

Return true if sync helper `name` is registered.

**Params**

* `name` **{String}**: sync helper name
* `returns` **{Boolean}**: Returns true if the sync helper is registered

**Example**

```js
if (app.hasHelper('foo')) {
  // do stuff
}
```

### [.hasAsyncHelper](index.js#L184)

Return true if async helper `name` is registered.

**Params**

* `name` **{String}**: Async helper name
* `returns` **{Boolean}**: Returns true if the async helper is registered

**Example**

```js
if (app.hasAsyncHelper('foo')) {
  // do stuff
}
```

### [.helperGroup](index.js#L207)

Register a namespaced helper group.

**Params**

* `helpers` **{Object|Array}**: Object, array of objects, or glob patterns.

**Example**

```js
// markdown-utils
app.helperGroup('mdu', {
  foo: function() {},
  bar: function() {},
});

// Usage:
// <%= mdu.foo() %>
// <%= mdu.bar() %>
```

## History

### v0.2.0

* adds support for passing helper groups as a function. For example, the `log` helper can be a function, but it can also have `log.warning` and `log.info` functions as properties.

## About

### Related projects

* [base-engines](https://www.npmjs.com/package/base-engines): Adds support for managing template engines to your base application. | [homepage](https://github.com/node-base/base-engines "Adds support for managing template engines to your base application.")
* [base-option](https://www.npmjs.com/package/base-option): Adds a few options methods to base, like `option`, `enable` and `disable`. See the readme… [more](https://github.com/node-base/base-option) | [homepage](https://github.com/node-base/base-option "Adds a few options methods to base, like `option`, `enable` and `disable`. See the readme for the full API.")
* [base-task](https://www.npmjs.com/package/base-task): base plugin that provides a very thin wrapper around [https://github.com/doowb/composer](https://github.com/doowb/composer) for adding task methods to… [more](https://github.com/node-base/base-task) | [homepage](https://github.com/node-base/base-task "base plugin that provides a very thin wrapper around <https://github.com/doowb/composer> for adding task methods to your application.")
* [base](https://www.npmjs.com/package/base): base is the foundation for creating modular, unit testable and highly pluggable node.js applications, starting… [more](https://github.com/node-base/base) | [homepage](https://github.com/node-base/base "base is the foundation for creating modular, unit testable and highly pluggable node.js applications, starting with a handful of common methods, like `set`, `get`, `del` and `use`.")

### Contributing

Pull requests and stars are always welcome. For bugs and feature requests, [please create an issue](../../issues/new).

### Building docs

_(This document was generated by [verb-generate-readme](https://github.com/verbose/verb-generate-readme) (a [verb](https://github.com/verbose/verb) generator), please don't edit the readme directly. Any changes to the readme must be made in [.verb.md](.verb.md).)_

To generate the readme and API documentation with [verb](https://github.com/verbose/verb):

```sh
$ npm install -g verb verb-generate-readme && verb
```

### Running tests

Install dev dependencies:

```sh
$ npm install -d && npm test
```

### Author

**Jon Schlinkert**

* [github/jonschlinkert](https://github.com/jonschlinkert)
* [twitter/jonschlinkert](https://twitter.com/jonschlinkert)

### License

Copyright © 2017, [Jon Schlinkert](https://github.com/jonschlinkert).
Released under the [MIT license](LICENSE).

***

_This file was generated by [verb-generate-readme](https://github.com/verbose/verb-generate-readme), v0.4.1, on January 17, 2017._