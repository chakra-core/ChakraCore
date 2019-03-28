# load-templates [![NPM version](https://img.shields.io/npm/v/load-templates.svg?style=flat)](https://www.npmjs.com/package/load-templates) [![NPM monthly downloads](https://img.shields.io/npm/dm/load-templates.svg?style=flat)](https://npmjs.org/package/load-templates)  [![NPM total downloads](https://img.shields.io/npm/dt/load-templates.svg?style=flat)](https://npmjs.org/package/load-templates) [![Linux Build Status](https://img.shields.io/travis/jonschlinkert/load-templates.svg?style=flat&label=Travis)](https://travis-ci.org/jonschlinkert/load-templates)

> Load templates/views using globs, file paths, objects, arrays, or key-value pairs.

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install --save load-templates
```

## Usage

In addition to what's shown in the below examples, if a glob pattern or valid filepath is passed, a `stat` object will be added to the `file` object as well.

```js
var Loader = require('load-templates');
var loader = new Loader([options]);
var views = loader.load('*.hbs');
console.log(views);
```

## Supported formats

```js
// filepath
loader.load('a/b/c/some-template.hbs'); 

// array of filepaths
loader.load([
  'a/b/c/some-template.hbs',
  'a/b/c/other-template.hbs'
]); 

// glob pattern
loader.load('*.hbs'); 

// array of globs
loader.load(['*.hbs', '*.tmpl']); 

// object
loader.load({path: 'foo'});

// key-value 
loader.load('d', {path: 'd'});
loader.load('e', {path: 'e'});
loader.load('f', {path: 'f'});

// object of objects
loader.load({
  a: {path: 'a'},
  b: {path: 'b'},
  c: {path: 'c'}
});

// array of objects
loader.load([
  {path: 'a'},
  {path: 'b'},
  {path: 'c'}
]);

// array of nested objects
loader.load([
  {
    a: {path: 'test/fixtures/a.md'},
    b: {path: 'test/fixtures/b.md'},
    c: {path: 'test/fixtures/c.md'},
  },
  {
    d: {path: 'test/fixtures/d.md'},
    e: {path: 'test/fixtures/e.md'},
    f: {path: 'test/fixtures/f.md'},
  }
]);
```

## Options

### options.cwd

Type: `String`

Default: `process.cwd()`

Pass the current working directory to use for resolving paths.

### options.renameKey

Type: `Function`

Default: `file.path` Absolute filepath

Function to modify `file.key`, which is the property used for setting views on `loader.cache`. It works something like this:

```js
loader.cache[file.key] = file;
```

## API

### [Loader](index.js#L27)

Create an instance of `Loader` with the given `options`.

**Params**

* `options` **{Object}**

**Example**

```js
var Loader = require('load-templates');
var loader = new Loader();
```

### [.load](index.js#L53)

Load one or more templates from a filepath, glob pattern, object, or array of filepaths, glob patterns or objects. This method detects the type of value to be handled then calls one of the other methods to do the actual loading.

**Params**

* `templates` **{Object}**
* `options` **{Object}**
* `returns` **{Object}**: Returns the views from `loader.cache`

**Example**

```js
var loader = new Loader();
console.log(loader.load(['foo/*.hbs', 'bar/*.hbs']));
console.log(loader.load({path: 'a/b/c.md'}));
console.log(loader.load('index', {path: 'a/b/c.md'}));
```

### [.createView](index.js#L94)

Create a `view` object from the given `template`. View objects are instances of [vinyl](https://github.com/gulpjs/vinyl).

**Params**

* `template` **{Object|String}**: Filepath or object with `path` or `contents` properties.
* `options` **{Object}**
* `returns` **{Object}**: Returns the view.

**Example**

```js
console.log(loader.createView('test/fixtures/foo/bar.hbs'));
console.log(loader.createView('foo/bar.hbs', {cwd: 'test/fixtures'}));
```

### [.addView](index.js#L144)

Create a view from the given `template` and cache it on `loader.cache`.

**Params**

* `template` **{String|Object}**
* `options` **{Object}**
* `returns` **{Object}**: Returns the `Loader` instance for chaining

**Example**

```js
var loader = new Loader();
loader.addView('foo.hbs');
console.log(loader.cache);
```

### [.addViews](index.js#L179)

Create from an array or object of `templates` and cache them on `loader.cache`.

**Params**

* `templates` **{Object}**
* `options` **{Object}**

**Example**

```js
var loader = new Loader();
loader.addViews([
  {path: 'test/fixtures/a.md'},
  {path: 'test/fixtures/b.md'},
  {path: 'test/fixtures/c.md'},
]);
loader.addViews({
  d: {path: 'test/fixtures/d.md'},
  e: {path: 'test/fixtures/e.md'},
  f: {path: 'test/fixtures/f.md'},
});
loader.addViews([{
  g: {path: 'test/fixtures/g.md'},
  h: {path: 'test/fixtures/h.md'},
  i: {path: 'test/fixtures/i.md'},
}]);
console.log(loader.cache);
```

### [.globViews](index.js#L226)

Load templates from one or more glob `patterns` with the given `options`, then cache them on `loader.cache`.

**Params**

* `patterns` **{String|Array}**
* `options` **{Object}**
* `returns` **{Object}**: Returns `loader.cache`

**Example**

```js
var loader = new Loader();
var views = loader.globViews('*.hbs', {cwd: 'templates'});
```

## About

### Related projects

* [assemble](https://www.npmjs.com/package/assemble): Get the rocks out of your socks! Assemble makes you fast at creating web projects… [more](https://github.com/assemble/assemble) | [homepage](https://github.com/assemble/assemble "Get the rocks out of your socks! Assemble makes you fast at creating web projects. Assemble is used by thousands of projects for rapid prototyping, creating themes, scaffolds, boilerplates, e-books, UI components, API documentation, blogs, building websit")
* [templates](https://www.npmjs.com/package/templates): System for creating and managing template collections, and rendering templates with any node.js template engine… [more](https://github.com/jonschlinkert/templates) | [homepage](https://github.com/jonschlinkert/templates "System for creating and managing template collections, and rendering templates with any node.js template engine. Can be used as the basis for creating a static site generator or blog framework.")
* [verb](https://www.npmjs.com/package/verb): Documentation generator for GitHub projects. Verb is extremely powerful, easy to use, and is used… [more](https://github.com/verbose/verb) | [homepage](https://github.com/verbose/verb "Documentation generator for GitHub projects. Verb is extremely powerful, easy to use, and is used on hundreds of projects of all sizes to generate everything from API docs to readmes.")

### Contributing

Pull requests and stars are always welcome. For bugs and feature requests, [please create an issue](../../issues/new).

### Contributors

| **Commits** | **Contributor** | 
| --- | --- |
| 217 | [jonschlinkert](https://github.com/jonschlinkert) |
| 2 | [doowb](https://github.com/doowb) |

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
MIT

***

_This file was generated by [verb-generate-readme](https://github.com/verbose/verb-generate-readme), v0.4.2, on February 03, 2017._