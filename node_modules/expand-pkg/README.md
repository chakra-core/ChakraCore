# expand-pkg [![NPM version](https://img.shields.io/npm/v/expand-pkg.svg?style=flat)](https://www.npmjs.com/package/expand-pkg) [![NPM downloads](https://img.shields.io/npm/dm/expand-pkg.svg?style=flat)](https://npmjs.org/package/expand-pkg) [![Linux Build Status](https://img.shields.io/travis/jonschlinkert/expand-pkg.svg?style=flat&label=Travis)](https://travis-ci.org/jonschlinkert/expand-pkg)

> Parse string values in package.json into objects.

You might also be interested in [normalize-pkg](https://github.com/jonschlinkert/normalize-pkg).

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install --save expand-pkg
```

## Usage

```js
var Config = require('./');
var config = new Config();
console.log(config.expand(require('./package')));
```

## Schema

Values are parsed using a [schema](lib/schema.js) that is passed to [map-schema](https://github.com/jonschlinkert/map-schema) (builds on the schema from [normalize-pkg](https://github.com/jonschlinkert/normalize-pkg) as a starting point):

* only properties that have a corresponding field on the schema will be parsed.
* any properties that do not have a corresponding field are returned unmodified.

See the [.field docs](#field) to learn how to add or overwrite a field on the schema.

## Defaults

A `default` value may optionally be defined when a `.field` is registered. When `.expand` is run and a property that is required or recommended by npm is missing, `expand-pkg` attempts to create the field if valid data can be found in the repository.

The following fields are the only built-in fields with default values:

* `version`: `'0.1.0'`
* `license`: `'MIT'`
* `engines`: `{node: '>= 0.10.0'}`

## API

### [Config](index.js#L24)

Create an instance of `Config` with the given `options`.

**Example**

```js
var config = new Config();
var pkg = config.expand({
  author: 'Jon Schlinkert (https://github.com/jonschlinkert)'
});
console.log(pkg);
//=> {name: 'Jon Schlinkert', url: 'https://github.com/jonschlinkert'}
```

**Params**

* `options` **{Object}**

### [.field](index.js#L70)

Add a field to the schema, or overwrite or extend an existing field. The last argument is an `options` object that supports the following properties:

* `normalize` **{Function}**: function to be called on the given package.json value when the `.expand` method is called
* `default` **{any}**: default value to be used when the package.json property is undefined.
* `required` **{Boolean}**: define `true` if the property is required

**Example**

```js
var config = new Config();

config.field('foo', 'string', {
  default: 'bar'
});

var pkg = config.expand({});
console.log(pkg);
//=> {foo:  'bar'}
```

**Params**

* `name` **{String}**: Field name (required)
* `type` **{String|Array}**: One or more native javascript types allowed for the property value (required)
* `options` **{Object}**
* `returns` **{Object}**: Returns the instance

### [.expand](index.js#L96)

Iterate over `pkg` properties and expand values that have corresponding [fields](#field) registered on the schema.

**Example**

```js
var config = new Config();
var pkg = config.expand(require('./package.json'));
```

**Params**

* `pkg` **{Object}**: The `package.json` object to expand
* `options` **{Object}**
* `returns` **{Object}**: Returns an expanded package.json object.

## Options

### options.knownOnly

**Type**: `boolean`

**Default**: `undefined`

Omit properties from package.json that do not have a field registered on the schema.

```js
var Config = require('expand-pkg');
var config = new Config({knownOnly: true});

console.log(config.expand({author: 'Brian Woodward', foo: 'bar'}));
//=> {author: {name: 'Brian Woodward'}}
```

### options.pick

**Type**: `array`

**Default**: `undefined`

Filter the resulting object to contain only the specified keys.

### options.omit

**Type**: `array`

**Default**: `undefined`

Remove the specified keys from the resulting object.

### options.fields

Pass a `fields` object on the options to customize any fields on the schema (also see [options.extend](#options-extend)):

```js
var pkg = config.expand(require('./package'), {
  extend: true,
  fields: {
    name: {
      normalize: function() {
        return 'bar'
      }
    }
  }
});

console.log(pkg.name);
//=> 'bar'
```

### options.extend

**Type**: `boolean`

**Default**: `undefined`

Used with [options.field](#options-field), pass `true` if you want to extend a field that is already defined on the schema.

```js
var pkg = config.expand(require('./package'), {
  extend: true,
  fields: {
    name: {
      normalize: function() {
        return 'bar'
      }
    }
  }
});

console.log(pkg.name);
//=> 'bar'
```

## About

### Related projects

* [normalize-pkg](https://www.npmjs.com/package/normalize-pkg): Normalize values in package.json using the map-schema library. | [homepage](https://github.com/jonschlinkert/normalize-pkg "Normalize values in package.json using the map-schema library.")
* [repo-utils](https://www.npmjs.com/package/repo-utils): Utils for normalizing and formatting repo data. | [homepage](https://github.com/jonschlinkert/repo-utils "Utils for normalizing and formatting repo data.")
* [sync-pkg](https://www.npmjs.com/package/sync-pkg): CLI to sync only basic properties from package.json to bower.json. | [homepage](https://github.com/jonschlinkert/sync-pkg "CLI to sync only basic properties from package.json to bower.json.")

### Contributing

Pull requests and stars are always welcome. For bugs and feature requests, [please create an issue](../../issues/new).

### Contributors

| **Commits** | **Contributor** | 
| --- | --- |
| 27 | [jonschlinkert](https://github.com/jonschlinkert) |
| 4 | [doowb](https://github.com/doowb) |

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
Released under the [MIT license](LICENSE).

***

_This file was generated by [verb-generate-readme](https://github.com/verbose/verb-generate-readme), v0.2.0, on December 02, 2016._