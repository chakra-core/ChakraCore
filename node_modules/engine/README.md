# engine [![NPM version](https://img.shields.io/npm/v/engine.svg?style=flat)](https://www.npmjs.com/package/engine) [![NPM downloads](https://img.shields.io/npm/dm/engine.svg?style=flat)](https://npmjs.org/package/engine) [![Build Status](https://img.shields.io/travis/jonschlinkert/engine.svg?style=flat)](https://travis-ci.org/jonschlinkert/engine)

Template engine based on Lo-Dash template, but adds features like the ability to register helpers and more easily set data to be used as context in templates.

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install --save engine
```

## Usage

```js
var Engine = require('engine');
var engine = new Engine();

engine.helper('upper', function(str) {
  return str.toUpperCase();
});

engine.render('<%= upper(name) %>', {name: 'Brian'});
//=> 'BRIAN'
```

## API

### [Engine](index.js#L28)

Create an instance of `Engine` with the given options.

**Params**

* `options` **{Object}**

**Example**

```js
var Engine = require('engine');
var engine = new Engine();

// or
var engine = require('engine')();
```

### [.helper](index.js#L82)

Register a template helper.

**Params**

* `prop` **{String}**
* `fn` **{Function}**
* `returns` **{Object}**: Instance of `Engine` for chaining

**Example**

```js
engine.helper('upper', function(str) {
  return str.toUpperCase();
});

engine.render('<%= upper(user) %>', {user: 'doowb'});
//=> 'DOOWB'
```

### [.helpers](index.js#L112)

Register an object of template helpers.

**Params**

* `helpers` **{Object|Array}**: Object or array of helper objects.
* `returns` **{Object}**: Instance of `Engine` for chaining

**Example**

```js
engine.helpers({
 upper: function(str) {
   return str.toUpperCase();
 },
 lower: function(str) {
   return str.toLowerCase();
 }
});

// Or, just require in `template-helpers`
engine.helpers(require('template-helpers')._);
```

### [.data](index.js#L131)

Add data to be passed to templates as context.

**Params**

* `key` **{String|Object}**: Property key, or an object
* `value` **{any}**: If key is a string, this can be any typeof value
* `returns` **{Object}**: Engine instance, for chaining

**Example**

```js
engine.data({first: 'Brian'});
engine.render('<%= last %>, <%= first %>', {last: 'Woodward'});
//=> 'Woodward, Brian'
```

### [.compile](index.js#L200)

Creates a compiled template function that can interpolate data properties in "interpolate" delimiters, HTML-escape interpolated data properties in "escape" delimiters, and execute JavaScript in "evaluate" delimiters. Data properties may be accessed as free variables in the template. If a setting object is provided it takes precedence over `engine.settings` values.

**Params**

* `str` **{string}**: The template string.
* `opts` **{Object}**: The options object.
* `escape` **{RegExp}**: The HTML "escape" delimiter.
* `evaluate` **{RegExp}**: The "evaluate" delimiter.
* `imports` **{Object}**: An object to import into the template as free variables.
* `interpolate` **{RegExp}**: The "interpolate" delimiter.
* `sourceURL` **{string}**: The sourceURL of the template's compiled source.
* `variable` **{string}**: The data object variable name.
* `returns` **{Function}**: Returns the compiled template function.

**Example**

```js
var fn = engine.compile('Hello, <%= user %>!');
//=> [function]

fn({user: 'doowb'});
//=> 'Hello, doowb!'

fn({user: 'halle'});
//=> 'Hello, halle!'
```

### [.render](index.js#L317)

Renders templates with the given data and returns a string.

**Params**

* `str` **{String}**
* `data` **{Object}**
* `returns` **{String}**

**Example**

```js
engine.render('<%= user %>', {user: 'doowb'});
//=> 'doowb'
```

## About

### Related projects

* [assemble](https://www.npmjs.com/package/assemble): Get the rocks out of your socks! Assemble makes you fast at creating web projects… [more](https://github.com/assemble/assemble) | [homepage](https://github.com/assemble/assemble "Get the rocks out of your socks! Assemble makes you fast at creating web projects. Assemble is used by thousands of projects for rapid prototyping, creating themes, scaffolds, boilerplates, e-books, UI components, API documentation, blogs, building websit")
* [template-helpers](https://www.npmjs.com/package/template-helpers): Generic JavaScript helpers that can be used with any template engine. Handlebars, Lo-Dash, Underscore, or… [more](https://github.com/jonschlinkert/template-helpers) | [homepage](https://github.com/jonschlinkert/template-helpers "Generic JavaScript helpers that can be used with any template engine. Handlebars, Lo-Dash, Underscore, or any engine that supports helper functions.")
* [template](https://www.npmjs.com/package/template): Render templates using any engine. Supports, layouts, pages, partials and custom template types. Use template… [more](https://github.com/jonschlinkert/template) | [homepage](https://github.com/jonschlinkert/template "Render templates using any engine. Supports, layouts, pages, partials and custom template types. Use template helpers, middleware, routes, loaders, and lots more. Powers assemble, verb and other node.js apps.")
* [verb](https://www.npmjs.com/package/verb): Documentation generator for GitHub projects. Verb is extremely powerful, easy to use, and is used… [more](https://github.com/verbose/verb) | [homepage](https://github.com/verbose/verb "Documentation generator for GitHub projects. Verb is extremely powerful, easy to use, and is used on hundreds of projects of all sizes to generate everything from API docs to readmes.")

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
Released under the [MIT license](https://github.com/jonschlinkert/engine/blob/master/LICENSE).

***

_This file was generated by [verb](https://github.com/verbose/verb), v0.9.0, on July 19, 2016._