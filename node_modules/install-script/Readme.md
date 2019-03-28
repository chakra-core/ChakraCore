# install-script [![Circle CI](https://circleci.com/gh/vdemedes/install-script.svg?style=svg)](https://circleci.com/gh/vdemedes/install-script)

Generate universal (OS X and Linux) installation scripts for your CLI programs hosted on npm.


### Features

- Installs git, curl and stable node (if needed)
- Globally installs your npm module (`npm install -g`)
- Generates understandable code (see [template.js](https://github.com/vdemedes/install-script/blob/master/template.ejs))


### Installation

```
$ npm install install-script --save
```


### Usage

See template for installation scripts in [template.js](https://github.com/vdemedes/install-script/blob/master/template.ejs).

```js
const installScript = require('install-script');

let script = installScript({
  name: 'ava'
});
```


### Related projects

- [install-script-cli](https://github.com/vdemedes/install-script-cli) - generate install scripts from CLI


### Tests

[![Circle CI](https://circleci.com/gh/vdemedes/install-script.svg?style=svg)](https://circleci.com/gh/vdemedes/install-script)

```
$ make test
```


### License

MIT © [vdemedes](https://github.com/vdemedes)
