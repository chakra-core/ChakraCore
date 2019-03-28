# async-helpers [![NPM version](https://img.shields.io/npm/v/async-helpers.svg?style=flat)](https://www.npmjs.com/package/async-helpers) [![NPM monthly downloads](https://img.shields.io/npm/dm/async-helpers.svg?style=flat)](https://npmjs.org/package/async-helpers)  [![NPM total downloads](https://img.shields.io/npm/dt/async-helpers.svg?style=flat)](https://npmjs.org/package/async-helpers) [![Linux Build Status](https://img.shields.io/travis/doowb/async-helpers.svg?style=flat&label=Travis)](https://travis-ci.org/doowb/async-helpers) [![Windows Build Status](https://img.shields.io/appveyor/ci/doowb/async-helpers.svg?style=flat&label=AppVeyor)](https://ci.appveyor.com/project/doowb/async-helpers)

> Use async helpers in templates with engines that typically only handle sync helpers. Handlebars and Lodash have been tested.

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install --save async-helpers
```

Install with [yarn](https://yarnpkg.com):

```sh
$ yarn add async-helpers
```

## Usage

```js
var asyncHelpers = require('async-helpers');
```

## API

### [AsyncHelpers](index.js#L32)

Create a new instance of AsyncHelpers

**Params**

* `options` **{Object}**: options to pass to instance
* `returns` **{Object}**: new AsyncHelpers instance

**Example**

```js
var asyncHelpers = new AsyncHelpers();
```

### [.set](index.js#L69)

Add a helper to the cache.

**Params**

* `name` **{String}**: Name of the helper
* `fn` **{Function}**: Helper function
* `returns` **{Object}**: Returns `this` for chaining

**Example**

```js
asyncHelpers.set('upper', function(str, cb) {
  cb(null, str.toUpperCase());
});
```

### [.get](index.js#L105)

Get all helpers or a helper with the given name.

**Params**

* `name` **{String}**: Optionally pass in a name of a helper to get.
* `options` **{Object}**: Additional options to use.
* `returns` **{Function|Object}**: Single helper function when `name` is provided, otherwise object of all helpers

**Example**

```js
var helpers = asyncHelpers.get();
var wrappedHelpers = asyncHelpers.get({wrap: true});
```

### [.wrapHelper](index.js#L125)

Wrap a helper with async handling capibilities.

**Params**

* `helper` **{String}**: Optionally pass the name of the helper to wrap
* `returns` **{Function|Object}**: Single wrapped helper function when `name` is provided, otherwise object of all wrapped helpers.

**Example**

```js
var wrappedHelper = asyncHelpers.wrap('upper');
var wrappedHelpers = asyncHelpers.wrap();
```

### [.reset](index.js#L232)

Reset all the stashed helpers.

* `returns` **{Object}**: Returns `this` to enable chaining

**Example**

```js
asyncHelpers.reset();
```

### [.resolveId](index.js#L281)

Resolve a stashed helper by the generated id. This is a generator function and should be used with [co](https://github.com/tj/co)

**Params**

* `key` **{String}**: ID generated when from executing a wrapped helper.

**Example**

```js
var upper = asyncHelpers.get('upper', {wrap: true});
var id = upper('doowb');

co(asyncHelpers.resolveId(id))
  .then(console.log)
  .catch(console.error);

//=> DOOWB
```

### [.resolveIds](index.js#L423)

After rendering a string using wrapped async helpers, use `resolveIds` to invoke the original async helpers and replace the async ids with results from the async helpers.

**Params**

* `str` **{String}**: String containing async ids
* `cb` **{Function}**: Callback function accepting an `err` and `content` parameters.

**Example**

```js
asyncHelpers.resolveIds(renderedString, function(err, content) {
  if (err) return console.error(err);
  console.log(content);
});
```

## About

### Related projects

* [assemble](https://www.npmjs.com/package/assemble): Get the rocks out of your socks! Assemble makes you fast at creating web projects… [more](https://github.com/assemble/assemble) | [homepage](https://github.com/assemble/assemble "Get the rocks out of your socks! Assemble makes you fast at creating web projects. Assemble is used by thousands of projects for rapid prototyping, creating themes, scaffolds, boilerplates, e-books, UI components, API documentation, blogs, building websit")
* [generate](https://www.npmjs.com/package/generate): Command line tool and developer framework for scaffolding out new GitHub projects. Generate offers the… [more](https://github.com/generate/generate) | [homepage](https://github.com/generate/generate "Command line tool and developer framework for scaffolding out new GitHub projects. Generate offers the robustness and configurability of Yeoman, the expressiveness and simplicity of Slush, and more powerful flow control and composability than either.")
* [templates](https://www.npmjs.com/package/templates): System for creating and managing template collections, and rendering templates with any node.js template engine… [more](https://github.com/jonschlinkert/templates) | [homepage](https://github.com/jonschlinkert/templates "System for creating and managing template collections, and rendering templates with any node.js template engine. Can be used as the basis for creating a static site generator or blog framework.")
* [update](https://www.npmjs.com/package/update): Be scalable! Update is a new, open source developer framework and CLI for automating updates… [more](https://github.com/update/update) | [homepage](https://github.com/update/update "Be scalable! Update is a new, open source developer framework and CLI for automating updates of any kind in code projects.")
* [verb](https://www.npmjs.com/package/verb): Documentation generator for GitHub projects. Verb is extremely powerful, easy to use, and is used… [more](https://github.com/verbose/verb) | [homepage](https://github.com/verbose/verb "Documentation generator for GitHub projects. Verb is extremely powerful, easy to use, and is used on hundreds of projects of all sizes to generate everything from API docs to readmes.")

### Contributing

Pull requests and stars are always welcome. For bugs and feature requests, [please create an issue](../../issues/new).

### Contributors

| **Commits** | **Contributor** |  
| --- | --- |  
| 85 | [doowb](https://github.com/doowb) |  
| 44 | [jonschlinkert](https://github.com/jonschlinkert) |  
| 1  | [nknapp](https://github.com/nknapp) |  

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

**Brian Woodward**

* [github/doowb](https://github.com/doowb)
* [twitter/doowb](https://twitter.com/doowb)

### License

Copyright © 2017, [Brian Woodward](https://github.com/doowb).
Released under the [MIT License](LICENSE).

***

_This file was generated by [verb-generate-readme](https://github.com/verbose/verb-generate-readme), v0.6.0, on December 13, 2017._