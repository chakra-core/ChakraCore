# fs-writefile-promise [![version][npm-version]][npm-url] [![License][npm-license]][license-url]

[Promise] version of [fs.writefile]:

> Asynchronously writes data to a file, replacing the file if it already exists.

[![Build Status][travis-image]][travis-url]
[![Downloads][npm-downloads]][npm-url]
[![Code Climate][codeclimate-quality]][codeclimate-url]
[![Coverage Status][codeclimate-coverage]][codeclimate-url]
[![Dependency Status][dependencyci-image]][dependencyci-url]
[![Dependencies][david-image]][david-url]

## Install

```bash
npm install --only=production --save fs-writefile-promise
```

## Usage

I recommend using an optimized build matching your Node.js environment version, otherwise, the standard `require` would work just fine with any version of Node `>= v4.0` .

```js
/*
 * Node 7
 */
const write = require('fs-writefile-promise/lib/node7')

/*
 * Node 6
 */
const write = require('fs-writefile-promise/lib/node6')

/*
 * Node 4 (Default)
 * Note: additional ES2015 polyfills may be required
 */
var write = require('fs-writefile-promise')
```

## API

### write(filename, data [, options])

*filename*: `String`
*data* `String` or `Buffer`
*options*: `Object`
Return: `Object` ([Promise])

When it finishes, it will be [*fulfilled*](http://promisesaplus.com/#point-26) with the file name that was written to.

When it fails, it will be [*rejected*](http://promisesaplus.com/#point-30) with an error as its first argument.

```js
write('/tmp/foo', 'bar')
  .then(function (filename) {
    console.log(filename) //=> '/tmp/foo'
  })

  .catch(function (err) {
    console.error(err)
  })
})
```

#### options

The option object will be directly passed to [fs.writefile](https://nodejs.org/api/fs.html#fs_fs_writefile_filename_data_options_callback).

----
> :copyright: [ahmadnassri.com](https://www.ahmadnassri.com/) &nbsp;&middot;&nbsp;
> License: [ISC][license-url] &nbsp;&middot;&nbsp;
> Github: [@ahmadnassri](https://github.com/ahmadnassri) &nbsp;&middot;&nbsp;
> Twitter: [@ahmadnassri](https://twitter.com/ahmadnassri)

[license-url]: http://choosealicense.com/licenses/isc/

[travis-url]: https://travis-ci.org/ahmadnassri/fs-writefile-promise
[travis-image]: https://img.shields.io/travis/ahmadnassri/fs-writefile-promise.svg?style=flat-square

[npm-url]: https://www.npmjs.com/package/fs-writefile-promise
[npm-license]: https://img.shields.io/npm/l/fs-writefile-promise.svg?style=flat-square
[npm-version]: https://img.shields.io/npm/v/fs-writefile-promise.svg?style=flat-square
[npm-downloads]: https://img.shields.io/npm/dm/fs-writefile-promise.svg?style=flat-square

[codeclimate-url]: https://codeclimate.com/github/ahmadnassri/fs-writefile-promise
[codeclimate-quality]: https://img.shields.io/codeclimate/github/ahmadnassri/fs-writefile-promise.svg?style=flat-square
[codeclimate-coverage]: https://img.shields.io/codeclimate/coverage/github/ahmadnassri/fs-writefile-promise.svg?style=flat-square

[david-url]: https://david-dm.org/ahmadnassri/fs-writefile-promise
[david-image]: https://img.shields.io/david/ahmadnassri/fs-writefile-promise.svg?style=flat-square

[dependencyci-url]: https://dependencyci.com/github/ahmadnassri/fs-writefile-promise
[dependencyci-image]: https://dependencyci.com/github/ahmadnassri/fs-writefile-promise/badge?style=flat-square

[fs.writefile]: https://nodejs.org/api/fs.html#fs_fs_writefile_filename_data_options_callback
[Promise]: http://promisesaplus.com/
