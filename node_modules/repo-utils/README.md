# repo-utils [![NPM version](https://img.shields.io/npm/v/repo-utils.svg?style=flat)](https://www.npmjs.com/package/repo-utils) [![NPM downloads](https://img.shields.io/npm/dm/repo-utils.svg?style=flat)](https://npmjs.org/package/repo-utils) [![Linux Build Status](https://img.shields.io/travis/jonschlinkert/repo-utils.svg?style=flat&label=Travis)](https://travis-ci.org/jonschlinkert/repo-utils)

> Utils for normalizing and formatting repo data.

You might also be interested in [parse-git-config](https://github.com/jonschlinkert/parse-git-config).

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install --save repo-utils
```

## Usage

```js
var repo = require('repo-utils');
```

## API

### [.name](index.js#L40)

Get the `name` for a repository from: - github repository path (`owner/project-name`) - github URL - absolute file path to a directory on the local file system (`.` and `''` may be used as aliases for the current working directory)

**Example**

```js
repo.name(process.cwd());
//=> 'repo-utils'
repo.name('.');
//=> 'repo-utils'
repo.name();
//=> 'repo-utils'

repo.name('https://github.com/jonschlinkert/repo-utils');
//=> 'repo-utils'
repo.name('jonschlinkert/repo-utils');
//=> 'repo-utils'
```

**Params**

* `cwd` **{String}**: Absolute file path or github URL
* `returns` **{String}**: Project name

### [.repository](index.js#L73)

Create a github repository string in the form of `owner/name`, from: - full github repository URL - object returned from `url.parse` - list of arguments in the form of `owner, name`

**Example**

```js
repo.repository('jonschlinkert', 'micromatch');
//=> 'jonschlinkert/micromatch'

repo.repository({owner: 'jonschlinkert', repository: 'micromatch'});
//=> 'jonschlinkert/micromatch'

repo.repository('https://github.com/jonschlinkert/micromatch');
//=> 'jonschlinkert/micromatch'
```

**Params**

* `owner` **{String}**: Repository owner
* `name` **{String}**: Repository name
* `returns` **{String}**: Reps

### [.homepage](index.js#L120)

Create a `homepage` URL from a github repository path or github repository URL.

**Example**

```js
repo.homepage('jonschlinkert/repo-utils');
//=> 'https://github.com/jonchlinkert/repo-utils'
```

**Params**

* `repository` **{String}**: Repository in the form of `owner/project-name`
* `options` **{Object}**
* `returns` **{String}**: Formatted github homepage url.

### [.issues](index.js#L175)

Create a GitHub `issues` URL.

**Example**

```js
repo.isses('jonschlinkert/micromatch');
//=> 'https://github.com/jonchlinkert/micromatch/issues'
```

**Params**

* `repository` **{String}**: Repository in the form of `owner/project-name` or full github project URL.
* `options` **{Object}**
* `returns` **{String}**

### [.bugs](index.js#L192)

Create a GitHub `bugs` URL. Alias for [.issues](#issues).

**Example**

```js
repo.bugs('jonschlinkert/micromatch');
//=> 'https://github.com/jonchlinkert/micromatch/issues'
```

**Params**

* `repository` **{String}**: Repository in the form of `owner/project-name`
* `options` **{Object}**
* `returns` **{String}**

### [.https](index.js#L210)

Create a github `https` URL.

**Example**

```js
repo.https('jonschlinkert/micromatch');
//=> 'https://github.com/jonchlinkert/micromatch'
```

**Params**

* `repository` **{String}**: Repository in the form of `owner/project-name`
* `options` **{Object|String}**: Options object or optional branch
* `branch` **{String}**: Optionally specify a branch
* `returns` **{String}**

### [.travis](index.js#L234)

Create a travis URL.

**Example**

```js
repo.travis('jonschlinkert/micromatch');
//=> 'https://travis-ci.org/jonschlinkert/micromatch'
```

**Params**

* `repository` **{String}**: Repository in the form of `owner/project-name`
* `options` **{Object|String}**: Options object or optional branch
* `branch` **{String}**: Optionally specify a branch
* `returns` **{String}**

### [.file](index.js#L256)

Create a URL for a file in a github repository.

**Example**

```js
repo.file('https://github.com/jonschlinkert/micromatch', 'README.md');
//=> 'https://raw.githubusercontent.com/jonschlinkert/micromatch/master/README.md'

repo.raw('jonschlinkert/micromatch', 'README.md');
//=> 'https://raw.githubusercontent.com/jonschlinkert/micromatch/master/README.md'
```

**Params**

* `repository` **{String}**: Repository in the form of `owner/project-name` or full GitHub repository URL.
* `branch` **{String}**: Optionally specify a branch
* `path` **{String}**: Path to the file, relative to the repository root.
* `returns` **{String}**

### [.raw](index.js#L282)

Create a github "raw" content URL.

**Example**

```js
repo.raw('https://github.com/jonschlinkert/micromatch', 'README.md');
//=> 'https://raw.githubusercontent.com/jonschlinkert/micromatch/master/README.md'

repo.raw('jonschlinkert/micromatch', 'README.md');
//=> 'https://raw.githubusercontent.com/jonschlinkert/micromatch/master/README.md'
```

**Params**

* `repository` **{String}**: Repository in the form of `owner/project-name`
* `options` **{Object|String}**: Options object or optional branch
* `branch` **{String}**: Optionally specify a branch
* `returns` **{String}**

### [.isGithubUrl](index.js#L305)

Return true if the given string looks like a github URL.

**Example**

```js
utils.isGithubUrl('https://github.com/whatever');
//=> true
utils.isGithubUrl('https://foo.com/whatever');
//=> false
```

**Params**

* `str` **{String}**: URL to test
* `returns` **{Boolean}**

### [.parseUrl](index.js#L343)

Parse a GitHub repository URL or repository `owner/project-name` into an object.

**Example**

```js
// see the tests for supported formats
repo.parse('https://raw.githubusercontent.com/jonschlinkert/micromatch/master/README.md');

// results in:
{ protocol: 'https:',
  slashes: true,
  hostname: 'raw.githubusercontent.com',
  host: 'raw.githubusercontent.com',
  pathname: 'https://raw.githubusercontent.com/foo/bar/master/README.md',
  path: '/foo/bar/master/README.md',
  href: 'https://raw.githubusercontent.com/foo/bar/master/README.md',
  owner: 'foo',
  name: 'bar',
  repo: 'foo/bar',
  repository: 'foo/bar',
  branch: 'master' }
```

**Params**

* `repositoryURL` **{String}**: Full repository URL, or repository path in the form of `owner/project-name`
* `options` **{Object}**
* `returns` **{Boolean}**

### [.expandUrl](index.js#L405)

Parse a GitHub `repository` path or URL by calling `repo.parseUrl()`, then expands it into an object of URLs. (the object also includes properties returned from `.parse()`). A file path maybe be passed as the second argument to include `raw` and `file` properties in the result.

**Example**

```js
// see the tests for supported formats
repo.expand('https://github.com/abc/xyz.git', 'README.md');

// results in:
{ protocol: 'https:',
  slashes: true,
  hostname: 'github.com',
  host: 'github.com',
  pathname: 'https://github.com/abc/xyz.git',
  path: '/abc/xyz.git',
  href: 'https://github.com/abc/xyz.git',
  owner: 'abc',
  name: 'xyz',
  repo: 'abc/xyz',
  repository: 'abc/xyz',
  branch: 'master',
  host_api: 'api.github.com',
  host_raw: 'https://raw.githubusercontent.com',
  api: 'https://api.github.com/repos/abc/xyz',
  tarball: 'https://api.github.com/repos/abc/xyz/tarball/master',
  clone: 'https://github.com/abc/xyz',
  zip: 'https://github.com/abc/xyz/archive/master.zip',
  https: 'https://github.com/abc/xyz',
  travis: 'https://travis-ci.org/abc/xyz',
  file: 'https://github.com/abc/xyz/blob/master/README.md',
  raw: 'https://raw.githubusercontent.com/abc/xyz/master/README.md' }
```

**Params**

* `repository` **{String}**
* `file` **{String}**: Optionally pass a repository file path.
* `returns` **{String}**

### [.gitConfigPath](index.js#L456)

Get the local git config path, or global if a local `.git` repository does not exist.

**Example**

```js
console.log(repo.gitConfigPath());
//=> /Users/jonschlinkert/dev/repo-utils/.git/config

// if local .git repo does not exist
console.log(repo.gitConfigPath());
/Users/jonschlinkert/.gitconfig

// get global path
console.log(repo.gitConfigPath('global'));
/Users/jonschlinkert/.gitconfig
```

**Params**

* `type` **{String}**: Pass `global` to get the global git config path regardless of whether or not a local repository exists.
* `returns` **{String}**: Returns the local or global git path

### [.gitConfig](index.js#L471)

Get and parse global git config.

**Example**

```js
console.log(repo.gitConfig());
```

**Params**

* `options` **{Object}**: To get a local `.git` config, pass `{type: 'local'}`
* `returns` **{Object}**

### [.owner](index.js#L517)

Get an owner string from the given object or string.

**Example**

```js
console.log(repo.owner(require('./package.json')));
//=> 'jonschlinkert'
```

**Params**

* `config` **{String|Object}**: If an object is passed, it must have a `repository`, `url` or `author` propert (looked for in that order), otherwise if a string is passed it must be parse-able by the [parseUrl](#parseUrl) method.
* `returns` **{String}**

### [.person](index.js#L562)

Normalize a "person" object. If a "person" string is passed (like `author`, `contributor` etc) it is parsed into an object, otherwise the object is returned.

**Example**

```js
console.log(repo.person('Brian Woodward (https://github.com/doowb)'));
//=> { name: 'Brian Woodward', url: 'https://github.com/doowb' }
console.log(repo.person({ name: 'Brian Woodward', url: 'https://github.com/doowb' }));
//=> { name: 'Brian Woodward', url: 'https://github.com/doowb' }
```

**Params**

* `val` **{String|Object}**
* `returns` **{Object}**

### [.author](index.js#L594)

Returns an `author` object from the given given config object. If `config.author` is a string it will be parsed into an object.

**Example**

```js
console.log(repo.author({
  author: 'Brian Woodward (https://github.com/doowb)'
}));
//=> { name: 'Brian Woodward', url: 'https://github.com/doowb' }

console.log(repo.author({
  name: 'Brian Woodward',
  url: 'https://github.com/doowb'
}));
//=> { name: 'Brian Woodward', url: 'https://github.com/doowb' }
```

**Params**

* `config` **{Object}**: Object with an `author` property
* `returns` **{Object}**

### [.authorName](index.js#L622)

Returns the `author.name` from the given config object. If `config.author` is a string it will be parsed into an object first.

**Example**

```js
console.log(repo.authorName({
  author: 'Brian Woodward (https://github.com/doowb)'
}));
//=> 'Brian Woodward'

console.log(repo.authorName({
  name: 'Brian Woodward',
  url: 'https://github.com/doowb'
}));
//=> 'Brian Woodward'
```

**Params**

* `config` **{Object}**: Object with an `author` property
* `returns` **{Object}**

### [.authorUrl](index.js#L647)

Returns the `author.url` from the given config object. If `config.author` is a string it will be parsed into an object first.

**Example**

```js
console.log(repo.authorUrl({
  author: 'Brian Woodward (https://github.com/doowb)'
}));
//=> 'https://github.com/doowb'

console.log(repo.authorUrl({
  name: 'Brian Woodward',
  url: 'https://github.com/doowb'
}));
//=> 'https://github.com/doowb'
```

**Params**

* `config` **{Object}**: Object with an `author` property
* `returns` **{Object}**

### [.authorEmail](index.js#L673)

Returns the `author.email` from the given config object. If `config.author` is a string it will be parsed into an object first.

**Example**

```js
console.log(repo.authorEmail({
author: 'Brian Woodward <brian.woodward@sellside.com> (https://github.com/doowb)'
}));
//=> 'brian.woodward@sellside.com'

console.log(repo.authorEmail({
  name: 'Brian Woodward',
  url: 'https://github.com/doowb',
email: 'brian.woodward@sellside.com'
}));
//=> 'brian.woodward@sellside.com'
```

**Params**

* `config` **{Object}**: Object with an `author` property
* `returns` **{Object}**

### [.authorUsername](index.js#L699)

Returns the `author.username` from the given config object. If `config.author` is a string it will be parsed into an object first.

**Example**

```js
console.log(repo.authorUsername({
author: 'Brian Woodward <brian.woodward@sellside.com> (https://github.com/doowb)'
}));
//=> 'doowb'

console.log(repo.authorUsername({
  name: 'Brian Woodward',
  url: 'https://github.com/doowb',
email: 'brian.woodward@sellside.com'
}));
//=> 'doowb'
```

**Params**

* `config` **{Object}**: Object with an `author` property
* `returns` **{Object}**

### [.username](index.js#L725)

Returns a `username` from the given config object, by first attempting to get `author.username`, then

**Example**

```js
console.log(repo.username({
author: 'Brian Woodward <brian.woodward@sellside.com> (https://github.com/doowb)'
}));
//=> 'doowb'

console.log(repo.username({
  name: 'Brian Woodward',
  url: 'https://github.com/doowb',
email: 'brian.woodward@sellside.com'
}));
//=> 'doowb'
```

**Params**

* `config` **{Object}**: Object with an `author` property
* `returns` **{Object}**

## Coverage

As of October 29, 2016:

```
Statements   : 100% ( 0/0 )
Branches     : 100% ( 0/0 )
Functions    : 100% ( 0/0 )
Lines        : 100% ( 0/0 )
```

## About

### Related projects

* [git-config-path](https://www.npmjs.com/package/git-config-path): Resolve the path to the user's local or global .gitconfig. | [homepage](https://github.com/jonschlinkert/git-config-path "Resolve the path to the user's local or global .gitconfig.")
* [parse-author](https://www.npmjs.com/package/parse-author): Parse a string into an object with `name`, `email` and `url` properties following npm conventions… [more](https://github.com/jonschlinkert/parse-author) | [homepage](https://github.com/jonschlinkert/parse-author "Parse a string into an object with `name`, `email` and `url` properties following npm conventions. Useful for the `authors` property in package.json or for parsing an AUTHORS file into an array of authors objects.")
* [parse-git-config](https://www.npmjs.com/package/parse-git-config): Parse `.git/config` into a JavaScript object. sync or async. | [homepage](https://github.com/jonschlinkert/parse-git-config "Parse `.git/config` into a JavaScript object. sync or async.")
* [project-name](https://www.npmjs.com/package/project-name): Get the name of a project, from package.json, git config, or basename of the current… [more](https://github.com/jonschlinkert/project-name) | [homepage](https://github.com/jonschlinkert/project-name "Get the name of a project, from package.json, git config, or basename of the current working directory.")

### Contributing

Pull requests and stars are always welcome. For bugs and feature requests, [please create an issue](../../issues/new).

### Contributors

| **Commits** | **Contributor**<br/> | 
| --- | --- |
| 35 | [jonschlinkert](https://github.com/jonschlinkert) |
| 1 | [tunnckoCore](https://github.com/tunnckoCore) |

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

_This file was generated by [verb-generate-readme](https://github.com/verbose/verb-generate-readme), v0.2.0, on October 29, 2016._