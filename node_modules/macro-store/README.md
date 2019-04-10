# macro-store [![NPM version](https://img.shields.io/npm/v/macro-store.svg?style=flat)](https://www.npmjs.com/package/macro-store) [![NPM downloads](https://img.shields.io/npm/dm/macro-store.svg?style=flat)](https://npmjs.org/package/macro-store) [![Build Status](https://img.shields.io/travis/doowb/macro-store.svg?style=flat)](https://travis-ci.org/doowb/macro-store)

Get and set macros created by commandline arguments.

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install --save macro-store
```

## Usage

Create a [parser](#parser) function that works like [yargs-parser](https://github.com/yargs/yargs-parser) or [minimist](https://github.com/substack/minimist) and handles [creating](#create-a-macro), [using](#use-a-macro), and [removing](#delete-a-macro) macros through command line arguments.

```js
var macros = require('macro-store');
var parser = macros('macro-store');
var args = parser(process.argv.slice(2));
//=> { _: ['foo', 'bar', 'baz'], verbose: true, cwd: 'qux', isMacro: 'qux' }
```

The returned `args` object also contains the [store methods](#store-1) to give implementors direct access to [setting](#set), [getting](#get), and [deleting](#del) macros in the store.

```js
args.set('foo', ['bar', 'baz', 'bang']);
args.get('foo');
//=> ['bar', 'baz', 'bang']
```

## CLI examples

The following examples are using the [example file](example.js) run at the command line with `node example.js`.
The objects returned may be used in implementing applications however they choose.

Jump to [the API documentation](#api) for implementation information.

### Create a macro

The following shows creating a simple macro called `foo`.

![image](https://raw.githubusercontent.com/doowb/macro-store/master/docs/set-simple-macro.png)

### Use a macro

The following shows using the `foo` macro, and that the resulting `args` object contains the expanded value.

![image](https://raw.githubusercontent.com/doowb/macro-store/master/docs/get-simple-macro.png)

### Create a complex macro

The following shows creating a complex macro called `qux` that includes options `--verbose` and `--cwd boop`.

![image](https://raw.githubusercontent.com/doowb/macro-store/master/docs/set-complex-macro.png)

### Using a complex macro

The following shows that using a complex macro is the same as a simple macro, but the `args` object contains the options `verbose: true` and `cwd: 'boop'`, which were set when creating the `qux` macro.

![image](https://raw.githubusercontent.com/doowb/macro-store/master/docs/get-complex-macro.png)

### Delete a macro

The following shows how to delete the macro `foo`. This only deletes `foo` and shows that `qux` is still set.

![image](https://raw.githubusercontent.com/doowb/macro-store/master/docs/delete-macro.png)

### Deleting all macros

The following shows how to delete all macros. This shows that `foo` and `qux` have been deleted so the `args` object will contain the exact values passed in from the command line.

![image](https://raw.githubusercontent.com/doowb/macro-store/master/docs/delete-all-macros.png)

## API

### [macros](index.js#L54)

Handle macro processing and storing for an array of arguments.

Set macros by specifying using the `--macro` or `--macro=set` option and a list of values.
Remove a macro by specifying `--macro=del` option and the macro name.
Default is to replace values in the `argv` array with stored macro values (if found).

**Params**

* `name` **{String}**: Custom name of the [data-store](https://github.com/jonschlinkert/data-store) to use. Defaults to 'macros'.
* `options` **{Object}**: Options to pass to the store to control the name or instance of the [data-store](https://github.com/jonschlinkert/data-store)
* `options.name` **{String}**: Name of the [data-store](https://github.com/jonschlinkert/data-store) to use for storing macros. Defaults to `macros`
* `options.store` **{Object}**: Instance of [data-store](https://github.com/jonschlinkert/data-store) to use. Defaults to `new DataStore(options.name)`
* `options.parser` **{Function}**: Custom argv parser to use. Defaults to [yargs-parser](https://github.com/yargs/yargs-parser)
* `returns` **{Function}**: argv parser to process macros

**Example**

```js
// create an argv parser
var parser = macros('custom-macro-store');

// parse the argv
var args = parser(process.argv.slice(2));
// => { _: ['foo'] }

// following input will produce the following results:
//
// Set 'foo' as ['bar', 'baz', 'bang']
// $ app --macro foo bar baz bang
// => { _: [ 'bar', 'baz', 'bang' ], macro: 'foo' }
//
// Use 'foo'
// $ app foo
// => { _: [ 'bar', 'baz', 'bang' ], isMacro: 'foo' }
//
// Remove the 'foo' macro
// $ app --macro --del foo
// => { _: [ 'foo' ], macro: 'del' }
```

### [parser](index.js#L76)

Parser function used to parse the argv array and process macros.

**Params**

* `argv` **{Array}**: Array of arguments to process
* `options` **{Object}**: Additional options to pass to the argv parser
* `returns` **{Object}** `args`: object [described above](#macros)

### [.Store](index.js#L122)

Exposes `Store` for low level access

**Example**

```js
var store = new macros.Store('custom-macro-store');
```

### [Store](lib/store.js#L30)

Initialize a new `Store` with the given `options`.

**Params**

* `options` **{Object}**
* `options.name` **{String}**: Name of the json file to use for storing macros. Defaults to 'macros'
* `options.store` **{Object}**: Instance of [data-store](https://github.com/jonschlinkert/data-store) to use. Allows complete control over where the store is located.

**Example**

```js
var macroStore = new Store();
//=> '~/data-store/macros.json'

var macroStore = new Store({name: 'abc'});
//=> '~/data-store/abc.json'
```

### [.set](lib/store.js#L56)

Set a macro in the store.

**Params**

* `key` **{String}**: Name of the macro to set.
* `arr` **{Array}**: Array of strings that the macro will resolve to.
* `returns` **{Object}** `this`: for chaining

**Example**

```js
macroStore.set('foo', ['foo', 'bar', 'baz']);
```

### [.get](lib/store.js#L78)

Get a macro from the store.

**Params**

* `name` **{String}**: Name of macro to get.
* `returns` **{String|Array}**: Array of tasks to get from a stored macro, or the original name when a stored macro does not exist.

**Example**

```js
var tasks = macroStore.get('foo');
//=> ['foo', 'bar', 'baz']

// returns input name when macro is not in the store
var tasks = macroStore.get('bar');
//=> 'bar'
```

### [.del](lib/store.js#L95)

Remove a macro from the store.

**Params**

* `name` **{String|Array}**: Name of a macro or array of macros to remove.
* `returns` **{Object}** `this`: for chaining

**Example**

```js
macroStore.del('foo');
```

## Release history

### key

Changelog entries are classified using the following labels _(from [keep-a-changelog](https://github.com/olivierlacan/keep-a-changelog)_):

* `added`: for new features
* `changed`: for changes in existing functionality
* `deprecated`: for once-stable features removed in upcoming releases
* `removed`: for deprecated features removed in this release
* `fixed`: for any bug fixes

Custom labels used in this changelog:

* `dependencies`: bumps dependencies
* `housekeeping`: code re-organization, minor edits, or other changes that don't fit in one of the other categories.

### v0.3.0

**changed**

* refactored parser to return an `args` object similar to [yargs-parser](https://github.com/yargs/yargs-parser) and [minimist](https://github.com/substack/minimist)
* handles `--macro` option for creating, getting, and removing macros from the `args` object.

### v0.2.0

**changed**

* refactored to export a function that creates a parser function.
* the parser function returns an object with the action taken and the processed arguments.

## About

### Related projects

* [data-store](https://www.npmjs.com/package/data-store): Easily get, set and persist config data. | [homepage](https://github.com/jonschlinkert/data-store "Easily get, set and persist config data.")
* [minimist](https://www.npmjs.com/package/minimist): parse argument options | [homepage](https://github.com/substack/minimist "parse argument options")
* [verb-generate-readme](https://www.npmjs.com/package/verb-generate-readme): Generate your project's readme with verb. Requires verb v0.9.0 or higher. | [homepage](https://github.com/verbose/verb-generate-readme "Generate your project's readme with verb. Requires verb v0.9.0 or higher.")
* [verb](https://www.npmjs.com/package/verb): Documentation generator for GitHub projects. Verb is extremely powerful, easy to use, and is used… [more](https://github.com/verbose/verb) | [homepage](https://github.com/verbose/verb "Documentation generator for GitHub projects. Verb is extremely powerful, easy to use, and is used on hundreds of projects of all sizes to generate everything from API docs to readmes.")
* [yargs-parser](https://www.npmjs.com/package/yargs-parser): the mighty option parser used by yargs | [homepage](https://github.com/yargs/yargs-parser#readme "the mighty option parser used by yargs")

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

**Brian Woodward**

* [github/doowb](https://github.com/doowb)
* [twitter/doowb](http://twitter.com/doowb)

### License

Copyright © 2016, [Brian Woodward](https://github.com/doowb).
Released under the [MIT license](https://github.com/doowb/macro-store/blob/master/LICENSE).

***

_This file was generated by [verb](https://github.com/verbose/verb), v0.9.0, on August 17, 2016._