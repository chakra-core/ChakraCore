# root-check [![Build Status](https://travis-ci.org/sindresorhus/root-check.svg?branch=master)](https://travis-ci.org/sindresorhus/root-check)

> Try to [downgrade the permissions](https://github.com/sindresorhus/downgrade-root) of a process with root privileges and [block access](https://github.com/sindresorhus/sudo-block) if it fails

![](https://github.com/sindresorhus/sudo-block/raw/master/screenshot.png)


## Install

```
$ npm install --save root-check
```


## Usage

```js
var rootCheck = require('root-check');

rootCheck();
```


## API

See the [`sudo-block` API](https://github.com/sindresorhus/sudo-block#api).


## License

MIT © [Sindre Sorhus](http://sindresorhus.com)
