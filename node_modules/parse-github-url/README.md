# parse-github-url [![NPM version](https://img.shields.io/npm/v/parse-github-url.svg?style=flat)](https://www.npmjs.com/package/parse-github-url) [![NPM downloads](https://img.shields.io/npm/dm/parse-github-url.svg?style=flat)](https://npmjs.org/package/parse-github-url) [![Build Status](https://img.shields.io/travis/jonschlinkert/parse-github-url.svg?style=flat)](https://travis-ci.org/jonschlinkert/parse-github-url)

> Parse a github URL into an object.

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install parse-github-url --save
```

**HEADS UP! Breaking changes in 0.3.0!!!**

See the [release history](#history) for details.

**Why another GitHub URL parser library?**

Seems like every lib I've found does too much, like both stringifying and parsing, or converts the URL from one format to another, only returns certain segments of the URL except for what I need, yields inconsistent results or has poor coverage.

## Usage

```js
var gh = require('parse-github-url');
gh('https://github.com/jonschlinkert/micromatch');
```

Results in:

```js
{
  "owner": "jonschlinkert",
  "name": "micromatch",
  "repo": "jonschlinkert/micromatch",
  "branch": "master"
}
```

## Example results

Generated results from test fixtures:

```js
// assemble/verb#1.2.3
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: '1.2.3' }

// assemble/verb#branch
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'branch' }

// assemble/verb
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// git+https://github.com/assemble/verb.git
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// git+ssh://github.com/assemble/verb.git
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// git://gh.pages.com/assemble/verb.git
{ host: 'gh.pages.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// git://github.assemble.com/assemble/verb.git
{ host: 'github.assemble.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// git://github.assemble.two.com/assemble/verb.git
{ host: 'github.assemble.two.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// git://github.com/assemble/verb
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// git://github.com/assemble/verb.git
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// git@gh.pages.com:assemble/verb.git
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// git@github.com:assemble/verb.git#1.2.3
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: '1.2.3' }

// git@github.com:assemble/verb.git#v1.2.3
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'v1.2.3' }

// git@github.com:assemble/verb.git
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// github:assemble/verb
{ host: 'assemble',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// http://github.com/assemble
{ host: 'github.com',
  owner: 'assemble',
  name: null,
  repo: null,
  repository: null,
  branch: 'master' }

// http://github.com/assemble/verb
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// http://github.com/assemble/verb.git
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// http://github.com/assemble/verb/tree
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'tree' }

// http://github.com/assemble/verb/tree/master
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// http://github.com/assemble/verb/tree/master/foo/bar
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master/foo/bar' }

// https://assemble.github.com/assemble/verb/somefile.tar.gz
{ host: 'assemble.github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'somefile.tar.gz' }

// https://assemble.github.com/assemble/verb/somefile.zip
{ host: 'assemble.github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'somefile.zip' }

// https://assemble@github.com/assemble/verb.git
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// https://gh.pages.com/assemble/verb.git
{ host: 'gh.pages.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// https://github.com/assemble/verb
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// https://github.com/assemble/verb.git
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// https://github.com/assemble/verb/blob/1.2.3/README.md
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: '1.2.3' }

// https://github.com/assemble/verb/blob/249b21a86400b38969cee3d5df6d2edf8813c137/README.md
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'blob' }

// https://github.com/assemble/verb/blob/master/assemble/index.js
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'master' }

// https://github.com/assemble/verb/tree/1.2.3
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: '1.2.3' }

// https://github.com/assemble/verb/tree/feature/1.2.3
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'feature/1.2.3' }

// https://github.com/repos/assemble/verb/tarball
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'tarball' }

// https://github.com/repos/assemble/verb/zipball
{ host: 'github.com',
  owner: 'assemble',
  name: 'verb',
  repo: 'assemble/verb',
  repository: 'assemble/verb',
  branch: 'zipball' }
```

## History

**v0.3.0**

To be more consistent with node.js/package.json conventions, the following properties were renamed in `v0.3.0`:

* `repo` is now `name` (project name)
* `repopath` is now `repository` (project repository)
* `user` is now `owner` (project owner or org)

## Related projects

You might also be interested in these projects:

* [github-short-url-regex](https://www.npmjs.com/package/github-short-url-regex): Regular expression (Regex) for matching github shorthand (user/repo#branch). | [homepage](https://github.com/regexps/github-short-url-regex)
* [is-git-url](https://www.npmjs.com/package/is-git-url): Regex to validate that a URL is a git url. | [homepage](https://github.com/jonschlinkert/is-git-url)
* [parse-github-short-url](https://www.npmjs.com/package/parse-github-short-url): Parse a github/npm shorthand (user/repo#branch or user/repo@version) URL into an object. | [homepage](https://github.com/tunnckocore/parse-github-short-url)

## Contributing

Pull requests and stars are always welcome. For bugs and feature requests, [please create an issue](https://github.com/jonschlinkert/parse-github-url/issues/new).

## Building docs

Generate readme and API documentation with [verb](https://github.com/verbose/verb):

```sh
$ npm install verb && npm run docs
```

Or, if [verb](https://github.com/verbose/verb) is installed globally:

```sh
$ verb
```

## Running tests

Install dev dependencies:

```sh
$ npm install -d && npm test
```

## Author

**Jon Schlinkert**

* [github/jonschlinkert](https://github.com/jonschlinkert)
* [twitter/jonschlinkert](http://twitter.com/jonschlinkert)

## License

Copyright © 2016, [Jon Schlinkert](https://github.com/jonschlinkert).
Released under the [MIT license](https://github.com/jonschlinkert/parse-github-url/blob/master/LICENSE).

***

_This file was generated by [verb](https://github.com/verbose/verb), v0.9.0, on April 17, 2016._