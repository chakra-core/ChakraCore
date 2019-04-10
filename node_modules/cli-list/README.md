# cli-list
> Break CLI lists into arrays

[![build status][travis-status]][travis] ![downloads][downloads]

Given a "CLI list" like so:
```
foo bar, baz --qux, oof
```
We can expect `process.argv` to be something such as:
```javascript
['foo', 'bar,', 'baz', '--qux,', 'oof']
```
If we run this through the `cli-list` function we can split it into sub-arrays where the commas are:
```javascript
[['foo', 'bar'], ['baz', '--qux'], ['oof']]
```
Theses arrays maintain the `process.argv` style, so they can be used in parity with things like minimist.

## Installation
```shell
$ npm install --save cli-list
```

## Usage
```javascript
var list = require('cli-list');
var opts = list(process.argv.slice(2));
```

ES6 + Minimist:
```javascript
import list from 'cli-list';
import minimist from 'minimist';
const opts = list(process.argv.slice(2)).map(item => minimist(item));
```

## Examples
Given:
```
$ test foo --bar, baz, --qux
```
Expect:
```
[['foo', '--bar'], ['baz'], ['--qux']]
```

## Credits
| ![jamen][avatar] |
|:---:|
| [Jamen Marzonie][github] |

  [avatar]: https://avatars.githubusercontent.com/u/6251703?v=3&s=125
  [github]: https://github.com/jamen
  [travis-status]: https://travis-ci.org/jamen/cli-list.svg
  [travis]: https://travis-ci.org/jamen/cli-list
  [downloads]: https://img.shields.io/npm/dm/cli-list.svg
