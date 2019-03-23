# expand-target [![NPM version](https://img.shields.io/npm/v/expand-target.svg?style=flat)](https://www.npmjs.com/package/expand-target) [![NPM downloads](https://img.shields.io/npm/dm/expand-target.svg?style=flat)](https://npmjs.org/package/expand-target) [![Build Status](https://img.shields.io/travis/jonschlinkert/expand-target.svg?style=flat)](https://travis-ci.org/jonschlinkert/expand-target)

Expand target definitions in a declarative configuration.

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install --save expand-target
```

## Usage

```js
var target = require('expand-target');
```

Write declarative "target" definitions similar in concept to those used by [grunt](http://gruntjs.com/) and [make](https://www.gnu.org/software/make/). This is useful for shared configs or to dynamically build-up a configuration that can be passed to any build system (even [gulp](http://gulpjs.com)!).

**Basic example**

```js
target({
  files: {
    'a/': ['*.js'],
    'b/': ['*.js'],
    'c/': ['*.js']
  }
});
```

**results in**

```js
{
  files: [
    {
      src: ['examples.js', 'index.js'],
      dest: 'a/'
    },
    {
      src: ['examples.js', 'index.js'],
      dest: 'b/'
    },
    {
      src: ['examples.js', 'index.js'],
      dest: 'c/'
    }
  ]
}
```

> the same example with `expand: true` defined on the options

```js
target({
  options: { expand: true },
  files: {
    'a/': ['*.js'],
    'b/': ['*.js'],
    'c/': ['*.js']
  }
});
```

**results in**

```js
{
  options: {
    expand: true
  },
  files: [
    {
      src: ['examples.js'],
      dest: 'a/examples.js'
    },
    {
      src: ['index.js'],
      dest: 'a/index.js'
    },
    {
      src: ['examples.js'],
      dest: 'b/examples.js'
    },
    {
      src: ['index.js'],
      dest: 'b/index.js'
    },
    {
      src: ['examples.js'],
      dest: 'c/examples.js'
    },
    {
      src: ['index.js'],
      dest: 'c/index.js'
    }
  ]
}
```

See [more examples](./examples.md). Visit [expand-files](https://github.com/jonschlinkert/expand-files) for the full range of options and documentation.

## Plugins

Plugins must be registered before the configuration is expanded, which means you need to use the `expand` method instead of passing your config directly to the constructor.

```js
var target = new Target({cwd: 'foo'});

target
  .use(foo)
  .use(bar)
  .use(baz);

target.expand({
  src: '*.js',
  dest: ''
});
```

### Writing plugins

Plugins are just functions where the only parameter exposed is the current "context".

**Examples**

In the following plugin, `config` is the target instance:

```js
target.use(function(config) {
  config.foo = 'bar';
});
console.log(target.foo);
//=> 'bar'
```

To have the plugin called in a "child" context, like for iterating over files nodes as they're expanded, just return the plugin function until you get the node you want:

```js
target.use(function fn(config) {
  if (!config.node) return fn;
  console.log(config);
});
```

**Contexts**

To see all available contexts, just do the following:

```js
target.use(function fn(config) {
  console.log('-----', config._name, '----');
  console.log(config);
  console.log('---------------------------');
  return fn;
});
```

## Options

Any option from [expand-files](https://github.com/jonschlinkert/expand-files) may be used. Please see that project for the full range of options and documentation.

### options properties

The below "special" properties are fine to use either on an `options` object or on the root of the object passed to `expand-files`.

Either way they will be normalized onto the `options` object to ensure that [globby](https://github.com/sindresorhus/globby) and consuming libraries are passed the correct arguments.

**special properties**

* `base`
* `cwd`
* `destBase`
* `expand`
* `ext`
* `extDot`
* `extend`
* `flatten`
* `rename`
* `process`
* `srcBase`

**example**

Both of the following will result in `expand` being on the `options` object.

```js
files({src: '*.js', dest: 'dist/', options: {expand: true}});
files({src: '*.js', dest: 'dist/', expand: true});
```

## About

### Related projects

* [expand-config](https://www.npmjs.com/package/expand-config): Expand tasks, targets and files in a declarative configuration. | [homepage](https://github.com/jonschlinkert/expand-config "Expand tasks, targets and files in a declarative configuration.")
* [expand-files](https://www.npmjs.com/package/expand-files): Expand glob patterns in a declarative configuration into src-dest mappings. | [homepage](https://github.com/jonschlinkert/expand-files "Expand glob patterns in a declarative configuration into src-dest mappings.")
* [expand-target](https://www.npmjs.com/package/expand-target): Expand target definitions in a declarative configuration. | [homepage](https://github.com/jonschlinkert/expand-target "Expand target definitions in a declarative configuration.")
* [expand-task](https://www.npmjs.com/package/expand-task): Expand and normalize task definitions in a declarative configuration. | [homepage](https://github.com/jonschlinkert/expand-task "Expand and normalize task definitions in a declarative configuration.")
* [files-objects](https://www.npmjs.com/package/files-objects): Expand files objects into src-dest mappings. | [homepage](https://github.com/jonschlinkert/files-objects "Expand files objects into src-dest mappings.")

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
Released under the [MIT license](https://github.com/jonschlinkert/expand-target/blob/master/LICENSE).

***

_This file was generated by [verb](https://github.com/verbose/verb), v0.9.0, on July 19, 2016._