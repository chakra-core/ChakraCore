# bower-latest 
[![NPM version][npm-image]][npm-url] [![Build Status][travis-image]][travis-url] [![Dependency Status][daviddm-url]][daviddm-image] [![Coverage Status][coveralls-image]][coveralls-url]

Quickly find the latest version of a package in bower.

## Install

```bash
$ npm install --save bower-latest
```


## Usage

```javascript
var bowerLatest = require('bower-latest');
bowerLatest('backbone', function(compontent){
  console.log(compontent.version);
});
```

## API

_(Coming soon)_


## Contributing

In lieu of a formal styleguide, take care to maintain the existing coding style. Add unit tests for any new or changed functionality. Lint and test your code using [gulp](http://gulpjs.com/).


## Release History

_(Nothing yet)_


## License

Copyright (c) 2014 Kentaro Wakayama. Licensed under the MIT license.



[npm-url]: https://npmjs.org/package/bower-latest
[npm-image]: https://badge.fury.io/js/bower-latest.svg
[travis-url]: https://travis-ci.org/kwakayama/bower-latest
[travis-image]: https://travis-ci.org/kwakayama/bower-latest.svg?branch=master
[daviddm-url]: https://david-dm.org/kwakayama/bower-latest.svg?theme=shields.io
[daviddm-image]: https://david-dm.org/kwakayama/bower-latest
[coveralls-url]: https://coveralls.io/r/kwakayama/bower-latest?branch=master
[coveralls-image]: https://coveralls.io/repos/kwakayama/bower-latest/badge.png?branch=master
