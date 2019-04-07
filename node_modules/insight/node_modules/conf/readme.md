# conf [![Build Status: Linux and macOS](https://travis-ci.org/sindresorhus/conf.svg?branch=master)](https://travis-ci.org/sindresorhus/conf) [![Build status: Windows](https://ci.appveyor.com/api/projects/status/n88jwh3aju39i0p2/branch/master?svg=true)](https://ci.appveyor.com/project/sindresorhus/conf/branch/master)

> Simple config handling for your app or module

All you have to care about is what to persist. This module will handle all the dull details like where and how.

*If you need this for Electron, check out [`electron-store`](https://github.com/sindresorhus/electron-store) instead.*


## Install

```
$ npm install conf
```


## Usage

```js
const Conf = require('conf');
const config = new Conf();

config.set('unicorn', '🦄');
console.log(config.get('unicorn'));
//=> '🦄'

// Use dot-notation to access nested properties
config.set('foo.bar', true);
console.log(config.get('foo'));
//=> {bar: true}

config.delete('unicorn');
console.log(config.get('unicorn'));
//=> undefined
```

Or [create a subclass](https://github.com/sindresorhus/electron-store/blob/master/index.js).


## API

Changes are written to disk atomically, so if the process crashes during a write, it will not corrupt the existing config.

### Conf([options])

Returns a new instance.

### options

#### defaults

Type: `Object`

Default config.

#### configName

Type: `string`<br>
Default: `config`

Name of the config file (without extension).

Useful if you need multiple config files for your app or module. For example, different config files between two major versions.

#### projectName

Type: `string`<br>
Default: The `name` field in the package.json closest to where `conf` is imported.

You only need to specify this if you don't have a package.json file in your project.

#### cwd

Type: `string`<br>
Default: System default [user config directory](https://github.com/sindresorhus/env-paths#pathsconfig)

**You most likely don't need this. Please don't use it unless you really have to.**

Overrides `projectName`.

The only use-case I can think of is having the config located in the app directory or on some external storage.

#### encryptionKey

Type: `string` `Buffer` `TypedArray` `DataView`<br>
Default: `undefined`

Note that this is **not intended for security purposes**, since the encryption key would be easily found inside a plain-text Node.js app.

Its main use is for obscurity. If a user looks through the config directory and finds the config file, since it's just a JSON file, they may be tempted to modify it. By providing an encryption key, the file will be obfuscated, which should hopefully deter any users from doing so.

It also has the added bonus of ensuring the config file's integrity. If the file is changed in any way, the decryption will not work, in which case the store will just reset back to its default state.

When specified, the store will be encrypted using the [`aes-256-cbc`](https://en.wikipedia.org/wiki/Block_cipher_mode_of_operation) encryption algorithm.

### Instance

You can use [dot-notation](https://github.com/sindresorhus/dot-prop) in a `key` to access nested properties.

The instance is [`iterable`](https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Iteration_protocols) so you can use it directly in a [`for…of`](https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Statements/for...of) loop.

#### .set(key, value)

Set an item.

The `value` must be JSON serializable.

#### .set(object)

Set multiple items at once.

#### .get(key, [defaultValue])

Get an item or `defaultValue` if the item does not exist.

#### .has(key)

Check if an item exists.

#### .delete(key)

Delete an item.

#### .clear()

Delete all items.

#### .onDidChange(key, callback)

`callback`: `(newValue, oldValue) => {}`

Watches the given `key`, calling `callback` on any changes. When a key is first set `oldValue` will be `undefined`, and when a key is deleted `newValue` will be `undefined`.

#### .size

Get the item count.

#### .store

Get all the config as an object or replace the current config with an object:

```js
conf.store = {
	hello: 'world'
};
```

#### .path

Get the path to the config file.



## FAQ

### How is this different from [`configstore`](https://github.com/yeoman/configstore)?

I'm also the author of `configstore`. While it's pretty good, I did make some mistakes early on that are hard to change at this point. This module is the result of everything I learned from making `configstore`. Mainly where config is stored. In `configstore`, the config is stored in `~/.config` (which is mainly a Linux convention) on all systems, while `conf` stores config in the system default [user config directory](https://github.com/sindresorhus/env-paths#pathsconfig). The `~/.config` directory, it turns out, often have an incorrect permission on macOS and Windows, which has caused a lot of grief for users.


## Related

- [electron-store](https://github.com/sindresorhus/electron-store) - Simple data persistence for your Electron app or module
- [cache-conf](https://github.com/SamVerschueren/cache-conf) - Simple cache config handling for your app or module


## License

MIT © [Sindre Sorhus](https://sindresorhus.com)
