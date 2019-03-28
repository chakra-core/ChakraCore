# base-npm [![NPM version](https://img.shields.io/npm/v/base-npm.svg?style=flat)](https://www.npmjs.com/package/base-npm) [![NPM downloads](https://img.shields.io/npm/dm/base-npm.svg?style=flat)](https://npmjs.org/package/base-npm) [![Build Status](https://img.shields.io/travis/node-base/base-npm.svg?style=flat)](https://travis-ci.org/node-base/base-npm)

Base plugin that adds methods for programmatically running npm commands.

You might also be interested in [base-bower](https://github.com/jonschlinkert/base-bower).

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install --save base-npm
```

## Usage

Note that if you use [base](https://github.com/node-base/base) directly you will also need to let the plugin know that it is being registered on a Base "application" (since `Base` can be used to create anything, like `views`, `collections` etc.).

```js
var npm = require('base-npm');
var Base = require('base');
var app = new Base({isApp: true}); // <=
app.use(npm());

// install npm packages `micromatch` and `is-absolute` to devDependencies
app.npm.devDependencies(['micromatch', 'is-absolute'], function(err) {
  if (err) throw err;
});
```

## API

### [.npm](index.js#L37)

Execute `npm install` with the given `args`, package `names` and callback.

**Params**

* `args` **{String|Array}**
* `names` **{String|Array}**
* `cb` **{Function}**: Callback

**Example**

```js
app.npm('--save', ['isobject'], function(err) {
  if (err) throw err;
});
```

### [.npm.install](index.js#L63)

Install one or more packages. Does not save anything to package.json. Equivalent of `npm install foo`.

**Params**

* `names` **{String|Array}**: package names
* `cb` **{Function}**: Callback

**Example**

```js
app.npm.install('isobject', function(err) {
  if (err) throw err;
});
```

### [.npm.latest](index.js#L81)

(Re-)install and save the latest version of all `dependencies` and `devDependencies` currently listed in package.json.

**Params**

* `cb` **{Function}**: Callback

**Example**

```js
app.npm.latest(function(err) {
  if (err) throw err;
});
```

### [.npm.dependencies](index.js#L115)

Execute `npm install --save` with one or more package `names`. Updates `dependencies` in package.json.

**Params**

* `names` **{String|Array}**
* `cb` **{Function}**: Callback

**Example**

```js
app.npm.dependencies('micromatch', function(err) {
  if (err) throw err;
});
```

### [.npm.devDependencies](index.js#L145)

Execute `npm install --save-dev` with one or more package `names`. Updates `devDependencies` in package.json.

**Params**

* `names` **{String|Array}**
* `cb` **{Function}**: Callback

**Example**

```js
app.npm.devDependencies('isobject', function(err) {
  if (err) throw err;
});
```

### [.npm.global](index.js#L174)

Execute `npm install --global` with one or more package `names`.

**Params**

* `names` **{String|Array}**
* `cb` **{Function}**: Callback

**Example**

```js
app.npm.global('mocha', function(err) {
  if (err) throw err;
});
```

### [.npm.exists](index.js#L201)

Check if one or more names exist on npm.

**Params**

* `names` **{String|Array}**
* `cb` **{Function}**: Callback
* `returns` **{Object}**: Object of results where the `key` is the name and the value is `true` or `false`.

**Example**

```js
app.npm.exists('isobject', function(err, results) {
  if (err) throw err;
  console.log(results.isobject);
});
//=> true
```

## History

**v0.4.1**

* fixes [issue #2](https://github.com/node-base/base-npm/issues/2) to use the `app.cwd` when available to ensure npm modules are installed to the correct folder

**v0.4.0**

* adds `global` method for installing with the `--global` flag
* adds `exists` method for checking if a package exists on `npm`
* removes [base-questions](https://github.com/node-base/base-questions)
* removes `askInstall` method (moved to [base-npm-prompt](https://github.com/node-base/base-npm-prompt))

**v0.3.0**

* improved instance checks
* adds [base-questions](https://github.com/node-base/base-questions)
* adds `dependencies` method
* adds `devDependencies` method

## About

### Related projects

* [base-questions](https://www.npmjs.com/package/base-questions): Plugin for base-methods that adds methods for prompting the user and storing the answers on… [more](https://github.com/node-base/base-questions) | [homepage](https://github.com/node-base/base-questions "Plugin for base-methods that adds methods for prompting the user and storing the answers on a project-by-project basis.")
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
* [twitter/jonschlinkert](http://twitter.com/jonschlinkert)

### License

Copyright © 2016, [Jon Schlinkert](https://github.com/jonschlinkert).
Released under the [MIT license](https://github.com/node-base/base-npm/blob/master/LICENSE).

***

_This file was generated by [verb-generate-readme](https://github.com/verbose/verb-generate-readme), v0.1.30, on September 11, 2016._