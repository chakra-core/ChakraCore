# engine-cache [![NPM version](https://img.shields.io/npm/v/engine-cache.svg?style=flat)](https://www.npmjs.com/package/engine-cache) [![NPM monthly downloads](https://img.shields.io/npm/dm/engine-cache.svg?style=flat)](https://npmjs.org/package/engine-cache)  [![NPM total downloads](https://img.shields.io/npm/dt/engine-cache.svg?style=flat)](https://npmjs.org/package/engine-cache) [![Linux Build Status](https://img.shields.io/travis/jonschlinkert/engine-cache.svg?style=flat&label=Travis)](https://travis-ci.org/jonschlinkert/engine-cache) [![Windows Build Status](https://img.shields.io/appveyor/ci/jonschlinkert/engine-cache.svg?style=flat&label=AppVeyor)](https://ci.appveyor.com/project/jonschlinkert/engine-cache)

> express.js inspired template-engine manager.

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install --save engine-cache
```

## Usage

```js
var Engines = require('engine-cache');
```

## API

### [Engines](index.js#L21)

**Params**

* `engines` **{Object}**: Optionally pass an object of engines to initialize with.

**Example**

```js
var Engines = require('engine-cache');
var engines = new Engines();
```

### [.setEngine](index.js#L43)

Register the given view engine callback `fn` as `ext`.

**Params**

* `ext` **{String}**
* `options` **{Object|Function}**: or callback `fn`.
* `fn` **{Function}**: Callback.
* `returns` **{Object}** `Engines`: to enable chaining.

**Example**

```js
var consolidate = require('consolidate')
engines.setEngine('hbs', consolidate.handlebars)
```

### [.setEngines](index.js#L74)

Add an object of engines onto `engines.cache`.

**Params**

* `obj` **{Object}**: Engines to load.
* `returns` **{Object}** `Engines`: to enable chaining.

**Example**

```js
engines.setEngines(require('consolidate'))
```

### [.getEngine](index.js#L101)

Return the engine stored by `ext`. If no `ext` is passed, undefined is returned.

**Params**

* `ext` **{String}**: The engine to get.
* `returns` **{Object}**: The specified engine.

**Example**

```js
var consolidate = require('consolidate');
var engine = engine.setEngine('hbs', consolidate.handlebars);

var engine = engine.getEngine('hbs');
console.log(engine);
// => {render: [function], renderFile: [function]}
```

### [.helpers](index.js#L132)

Get and set helpers for the given `ext` (engine). If no `ext` is passed, the entire helper cache is returned.

**Example:**

See [helper-cache](https://github.com/jonschlinkert/helper-cache) for any related issues, API details, and documentation.

**Params**

* `ext` **{String}**: The helper cache to get and set to.
* `returns` **{Object}**: Object of helpers for the specified engine.

**Example**

```js
var helpers = engines.helpers('hbs');
helpers.addHelper('bar', function() {});
helpers.getEngineHelper('bar');
helpers.getEngineHelper();
```

## Changelog

**v0.19.0** ensure the string is only rendered once by passing the compiled function to the `render` method

**v0.18.0** the `.load` method was renamed to `.setHelpers`

**v0.16.0** the `.clear()` method was removed. A custom `inspect` method was added.

**v0.15.0** `.getEngine()` no longer returns the entire `cache` object when `ext` is undefined.

## About

### Related projects

* [assemble](https://www.npmjs.com/package/assemble): Get the rocks out of your socks! Assemble makes you fast at creating web projects… [more](https://github.com/assemble/assemble) | [homepage](https://github.com/assemble/assemble "Get the rocks out of your socks! Assemble makes you fast at creating web projects. Assemble is used by thousands of projects for rapid prototyping, creating themes, scaffolds, boilerplates, e-books, UI components, API documentation, blogs, building websit")
* [async-helpers](https://www.npmjs.com/package/async-helpers): Use async helpers in templates with engines that typically only handle sync helpers. Handlebars and… [more](https://github.com/doowb/async-helpers) | [homepage](https://github.com/doowb/async-helpers "Use async helpers in templates with engines that typically only handle sync helpers. Handlebars and Lodash have been tested.")
* [engines](https://www.npmjs.com/package/engines): Template engine library with fast, synchronous rendering, based on consolidate. | [homepage](https://github.com/assemble/engines "Template engine library with fast, synchronous rendering, based on consolidate.")
* [helper-cache](https://www.npmjs.com/package/helper-cache): Easily register and get helper functions to be passed to any template engine or node.js… [more](https://github.com/jonschlinkert/helper-cache) | [homepage](https://github.com/jonschlinkert/helper-cache "Easily register and get helper functions to be passed to any template engine or node.js application. Methods for both sync and async helpers.")
* [templates](https://www.npmjs.com/package/templates): System for creating and managing template collections, and rendering templates with any node.js template engine… [more](https://github.com/jonschlinkert/templates) | [homepage](https://github.com/jonschlinkert/templates "System for creating and managing template collections, and rendering templates with any node.js template engine. Can be used as the basis for creating a static site generator or blog framework.")

### Contributing

Pull requests and stars are always welcome. For bugs and feature requests, [please create an issue](../../issues/new).

### Contributors

| **Commits** | **Contributor** | 
| --- | --- |
| 113 | [jonschlinkert](https://github.com/jonschlinkert) |
| 48 | [doowb](https://github.com/doowb) |

### Building docs

_(This project's readme.md is generated by [verb](https://github.com/verbose/verb-generate-readme), please don't edit the readme directly. Any changes to the readme must be made in the [.verb.md](.verb.md) readme template.)_

To generate the readme, run the following command:

```sh
$ npm install -g verbose/verb#dev verb-generate-readme && verb
```

### Running tests

Install dev dependencies:

```sh
$ npm install && npm test
```

### Author

**Jon Schlinkert**

* [github/jonschlinkert](https://github.com/jonschlinkert)
* [twitter/jonschlinkert](https://twitter.com/jonschlinkert)

### License

Copyright © 2017, [Jon Schlinkert](https://github.com/jonschlinkert).
Released under the [MIT license](LICENSE).

***

_This file was generated by [verb-generate-readme](https://github.com/verbose/verb-generate-readme), v0.4.2, on February 21, 2017._