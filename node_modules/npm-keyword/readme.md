# npm-keyword [![Build Status](https://travis-ci.org/sindresorhus/npm-keyword.svg?branch=master)](https://travis-ci.org/sindresorhus/npm-keyword)

> Get a list of npm packages with a certain keyword


## Install

```
$ npm install npm-keyword
```


## Usage

```js
const npmKeyword = require('npm-keyword');

(async () => {
	console.log(await npmKeyword('gulpplugin'));
	//=> [{name: 'gulp-autoprefixer', description: '…'}, …]

	console.log(await npmKeyword.names('gulpplugin'));
	//=> ['gulp-autoprefixer', …]

	console.log(await npmKeyword.count('gulpplugin'));
	//=> 3457
})();
```

## Caveat

The list of packages will contain a maximum of 250 packages matching the keyword. This limitation is caused by the [npm registry API](https://github.com/npm/registry/blob/master/docs/REGISTRY-API.md#get-v1search).


## API

### npmKeyword(keyword, [options])

Returns a promise for a list of packages having the specified keyword in their package.json `keyword` property.

#### keyword

Type: `string` `string[]`<br>
Example: `['string', 'camelcase']`

One or more keywords. Only matches packages that have *all* the given keywords.

#### options

Type: `object`

##### size

Type: `number`<br>
Default: `250`

Limits the amount of results.

### npmKeyword.names(keyword, [options])

Returns a promise for a list of package names. Use this if you don't need the description as it's faster.

#### keyword

Type: `string` `string[]`<br>
Example: `['string', 'camelcase']`

One or more keywords. Only matches packages that have *all* the given keywords.

#### options

Type: `object`

##### size

Type: `number`<br>
Default: `250`

Limits the amount of results.

### npmKeyword.count(keyword)

Returns a promise for the count of packages.

#### keyword

Type: `string` `string[]`<br>
Example: `['string', 'camelcase']`

One or more keywords. Only matches packages that have *all* the given keywords.


## Related

- [package-json](https://github.com/sindresorhus/package-json) - Get the package.json of a package from the npm registry
- [npm-user](https://github.com/sindresorhus/npm-user) - Get user info of an npm user
- [npm-email](https://github.com/sindresorhus/npm-email) - Get the email of an npm user


## License

MIT © [Sindre Sorhus](https://sindresorhus.com)
