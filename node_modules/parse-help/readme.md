# parse-help [![Build Status](https://travis-ci.org/sindresorhus/parse-help.svg?branch=master)](https://travis-ci.org/sindresorhus/parse-help)

> Parse CLI help output


## Install

```
$ npm install parse-help
```


## Usage

```js
const parseHelp = require('parse-help');

const help = `
	Usage
	  $ unicorn <name>

	Options
	  --rainbow    Lorem ipsum dolor sit amet
	  -m, --magic  Aenean commodo ligula eget dolor
	  --pony       Nullam dictum felis eu pede
	  -c, --color  Donec quam felis

	Examples
	  $ unicorn Peachy
	  $ unicorn Sparkle --rainbow --magic
`;

parseHelp(help);
/*
{
	flags: {
		rainbow: {
			description: 'Lorem ipsum dolor sit amet'
		},
		magic: {
			alias: 'm',
			description: 'Aenean commodo ligula eget dolor'
		},
		pony: {
			description: 'Nullam dictum felis eu pede'
		},
		color: {
			alias: 'c',
			description: 'Donec quam felis'
		}
	},
	aliases: {
		m: 'magic',
		c: 'color'
	}
}
*/
```


## Related

- [aliases](https://github.com/sindresorhus/aliases) - Parse flag aliases in CLI help output


## License

MIT © [Sindre Sorhus](https://sindresorhus.com)
