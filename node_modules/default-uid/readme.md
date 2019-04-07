# default-uid [![Build Status](https://travis-ci.org/sindresorhus/default-uid.svg?branch=master)](https://travis-ci.org/sindresorhus/default-uid)

> Get the system default [UID *(User ID)*](http://www.linfo.org/uid.html)


## Install

```
$ npm install --save default-uid
```


## Usage

```js
var defaultUid = require('default-uid');

// on OS X

defaultUid();
//=> 501

defaultUid('linux');
//=> 1000
```


## API

### defaultUid([platform])

#### platform

Type: `string`  
Default: `process.platform`

One of the [supported Node platforms](http://nodejs.org/api/process.html#process_process_platform).


## License

MIT © [Sindre Sorhus](http://sindresorhus.com)
