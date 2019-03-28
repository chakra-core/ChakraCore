# assemble-handle [![NPM version](https://img.shields.io/npm/v/assemble-handle.svg?style=flat)](https://www.npmjs.com/package/assemble-handle) [![NPM monthly downloads](https://img.shields.io/npm/dm/assemble-handle.svg?style=flat)](https://npmjs.org/package/assemble-handle) [![NPM total downloads](https://img.shields.io/npm/dt/assemble-handle.svg?style=flat)](https://npmjs.org/package/assemble-handle) [![Linux Build Status](https://img.shields.io/travis/assemble/assemble-handle.svg?style=flat&label=Travis)](https://travis-ci.org/assemble/assemble-handle)

> Assemble pipeline plugin for handling custom middleware stages.

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install --save assemble-handle
```

## Usage

```js
var handle = require('assemble-handle');
```

### handle

Handle middleware for the given middleware "stage".

```js
app.task('default', function() {
  return app.src('*.js')
    .pipe(handle(app, 'handlerName')) //<= handle middleware
    .pipe(app.dest('foo'))
});
```

**Example**

```js
var assemble = require('assemble');
var handle = require('assemble-handle');
var app = assemble();

/**
 * create some middleware "stages"
 */

app.handler('onStream');
app.handler('preWrite');
app.handler('postWrite');

/**
 * Create middleware
 */

app.onStream(/\.(js|css)$/, function(file, next) {
  // lint javascript
  next();
});

app.preWrite(/\.(jpg|png)$/, function(file, next) {
  // minify images
  next();
});

app.postWrite(/./, function(file, next) {
  // create files tree or something
  next();
});

/**
 * Run (handle) the middleware 
 */

app.task('site', function() {
  return app.src('assets/**/*.*')
    .pipe(handle(app, 'onStream'))  // handle onStream
    .pipe(handle(app, 'preWrite'))  // handle preWrite
    .pipe(app.dest('site/'));
    .pipe(handle(app, 'postWrite')) // handle postWrite
});
```

### handle.once

A `.once` method is exposed, which has the same exact behavior as the main function, but will ensure that middleware is only handled once for a given "stage".

**Example**

For example the given middleware will only run once.

```js
var assemble = require('assemble-core');
var handle = require('assemble-handle');
var app = assemble();

app.handler('onFoo');

app.onFoo(/./, function(file, next) {
  file.count = file.count || 0;
  file.count++;
  next();
});

app.task('handle-once', function(cb) {
  var files = [];
  app.src('test/**/*.*')
    .pipe(handle.once(app, 'onFoo'))
    .pipe(handle.once(app, 'onFoo'))
    .pipe(handle.once(app, 'onFoo'))
    .pipe(handle.once(app, 'onFoo'))
    .pipe(handle.once(app, 'onFoo'))
    .on('data', function(file) {
      files.push(file);
    })
    .pipe(app.dest('test/actual'))
    .on('end', function() {
      console.log(files[0].count);
      //=> 1
      cb();
    });
});

app.task('handle', function(cb) {
  var files = [];
  app.src('test/**/*.*')
    .pipe(handle(app, 'onFoo'))
    .pipe(handle(app, 'onFoo'))
    .pipe(handle(app, 'onFoo'))
    .pipe(handle(app, 'onFoo'))
    .pipe(handle(app, 'onFoo'))
    .on('data', function(file) {
      files.push(file);
    })
    .pipe(app.dest('test/actual'))
    .on('end', function() {
      console.log(files[0].count);
      //=> 5
      cb();
    });
});
```

## Custom handlers

Create custom middleware handlers.

```js
app.handler('onFoo');
```

This adds an `.onFoo` method to the instance:

```js
app.onFoo(/\.hbs$/, function(file, next) {
  // do stuff to file
  next();
});
```

All `.onFoo` middleware will now run when the `onFoo` handler is called:

```js
app.task('default', function() {
  return app.src('*.hbs')

    // call the `onFoo` handler
    .pipe(handle(app, 'onFoo')) 
});
```

## About

### Related projects

* [assemble-core](https://www.npmjs.com/package/assemble-core): The core assemble application with no presets or defaults. All configuration is left to the… [more](https://github.com/assemble/assemble-core) | [homepage](https://github.com/assemble/assemble-core "The core assemble application with no presets or defaults. All configuration is left to the implementor.")
* [assemble-fs](https://www.npmjs.com/package/assemble-fs): Assemble plugin to add methods to assemble for working with the file system, like src… [more](https://github.com/assemble/assemble-fs) | [homepage](https://github.com/assemble/assemble-fs "Assemble plugin to add methods to assemble for working with the file system, like src, dest, copy and symlink.")
* [assemble-streams](https://www.npmjs.com/package/assemble-streams): Assemble pipeline plugin for pushing views into a vinyl stream. | [homepage](https://github.com/assemble/assemble-streams "Assemble pipeline plugin for pushing views into a vinyl stream.")
* [assemble](https://www.npmjs.com/package/assemble): Get the rocks out of your socks! Assemble makes you fast at creating web projects… [more](https://github.com/assemble/assemble) | [homepage](https://github.com/assemble/assemble "Get the rocks out of your socks! Assemble makes you fast at creating web projects. Assemble is used by thousands of projects for rapid prototyping, creating themes, scaffolds, boilerplates, e-books, UI components, API documentation, blogs, building websit")

### Contributing

Pull requests and stars are always welcome. For bugs and feature requests, [please create an issue](../../issues/new).

### Contributors

| **Commits** | **Contributor** | 
| --- | --- |
| 13 | [jonschlinkert](https://github.com/jonschlinkert) |
| 3 | [doowb](https://github.com/doowb) |

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

_This file was generated by [verb-generate-readme](https://github.com/verbose/verb-generate-readme), v0.6.0, on May 28, 2017._