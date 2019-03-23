spawn-args
==========

![Build status](https://api.travis-ci.org/binocarlos/spawn-args.png)

Turn a string of command line options into an array for child_process.spawn

## install

```
$ npm install spawn-args
```

## usage

```js
var spawnargs = require('spawn-args');
//spawnargs(argString:string[, options:object]);

var args = spawnargs('-port 80 --title "this is a title"');

/*

	[
		'-port',
		'80',
		'--title',
		'"this is a title"'
	]
	
*/
```

The `removequotes` option will remove quotes from values if they do not have spaces

```js
var args2 = spawnargs('-port 80 --color "red" --title "this is a title"', { removequotes: true });

/*

	[
		'-port',
		'80',
		'--title',
		'"this is a title"'
	]
	
*/
```

If `removequotes` is `always` then quotes will be removed even if the value contains spaces

```js
var args3 = spawnargs('-port 80 --color "red" --title "this is a title"', { removequotes: 'always' });

/*

	[
		'-port',
		'80',
		'--title',
		'this is a title'
	]
	
*/
```

## license

MIT
