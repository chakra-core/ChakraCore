# fullname [![Build Status](https://travis-ci.org/sindresorhus/fullname.svg?branch=master)](https://travis-ci.org/sindresorhus/fullname)

> Get the fullname of the current user


## Install

```
$ npm install --save fullname
```

Tested on macOS, Linux, and Windows.


## Usage

```js
const fullname = require('fullname');

fullname().then(name => {
	console.log(name);
	//=> 'Sindre Sorhus'
});
```

In the rare case a name can't be found, you could fall back to [`username`](https://github.com/sindresorhus/username).


## Related

- [fullname-cli](https://github.com/sindresorhus/fullname-cli) - CLI for this module
- [fullname-native](https://github.com/sindresorhus/fullname-native) - Native version of this module
- [username](https://github.com/sindresorhus/username) - Get the username of the current user


## License

MIT © [Sindre Sorhus](https://sindresorhus.com)
