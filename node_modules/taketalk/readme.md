# taketalk

> Ever wanted a bin for your node module?


## Install

```
$ npm install --save taketalk
```


## Usage

```js
#!/usr/bin/env node

require('taketalk')({
  init: function (input, options) {
    console.log('This is the input from the CLI:', input);
    console.log('These are the options passed:', options);
  },

  help: 'Help yaself!' || function () {
    console.log('Print this when a user wants help.');
  },

  version: '0.1.1' || function () {
    console.log('Print this when a user asks for the version.');
  }
});
```


## License

MIT © Stephen Sawchuk
