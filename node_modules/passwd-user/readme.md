# passwd-user [![Build Status](https://travis-ci.org/sindresorhus/passwd-user.svg?branch=master)](https://travis-ci.org/sindresorhus/passwd-user)

> Get the [passwd](http://en.wikipedia.org/wiki/Passwd) user entry from a username or [uid](http://en.wikipedia.org/wiki/User_identifier_(Unix))

Works on OS X and Linux. See [`user-info`](https://github.com/sindresorhus/user-info) if you need cross-platform support.


## Instal

```
$ npm install --save passwd-user
```


## Usage

```js
const passwdUser = require('passwd-user');

passwdUser('sindresorhus').then(user => {
	console.log(user);
	/*
	{
		username: 'sindresorhus',
		password: '*',
		uid: 501,
		gid: 20,
		fullname: 'Sindre Sorhus',
		homedir: '/home/sindresorhus',
		shell: '/bin/zsh'
	}
	*/
});

// or
passwdUser(501).then(user => {
	console.log('Got entry for user 501');
});

// or
passwdUser().then(user => {
	console.log(`Got entry for user ${user.uid}`);
});
```


## API

Accepts a `username` or [`uid` number](https://en.wikipedia.org/wiki/User_identifier). Defaults to the current user ([`process.getuid()`](https://nodejs.org/api/process.html#process_process_getuid)).

Returns an object with:

- `username`
- `password`
- `uid`: user ID
- `gid`: group ID
- `fullname`: name of user
- `homedir`: home directory
- `shell`: default shell

### passwdUser([username | uid])

Returns a promise for an object with the user entry.

### passwdUser.sync([username | uid])

Returns an object with the user entry.


## Related

- [username](https://github.com/sindresorhus/username) - Get the user's username *(cross-platform)*
- [fullname](https://github.com/sindresorhus/fullname) - Get the user's fullname *(cross-platform)*


## License

MIT © [Sindre Sorhus](https://sindresorhus.com)
