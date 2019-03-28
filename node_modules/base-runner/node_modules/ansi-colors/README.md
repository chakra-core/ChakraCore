# ansi-colors [![NPM version](https://badge.fury.io/js/ansi-colors.svg)](http://badge.fury.io/js/ansi-colors)

> Collection of ansi colors and styles.

## Install

Install with [npm](https://www.npmjs.com/)

```sh
$ npm i ansi-colors --save
```

## Usage

```js
var colors = require('ansi-colors');
```

## API

### [bgblack](index.js#L47)

Wrap a string with ansi codes to create a black background.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.bgblack('some string'));
```

### [bgblue](index.js#L62)

Wrap a string with ansi codes to create a blue background.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.bgblue('some string'));
```

### [bgcyan](index.js#L77)

Wrap a string with ansi codes to create a cyan background.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.bgcyan('some string'));
```

### [bggreen](index.js#L92)

Wrap a string with ansi codes to create a green background.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.bggreen('some string'));
```

### [bgmagenta](index.js#L107)

Wrap a string with ansi codes to create a magenta background.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.bgmagenta('some string'));
```

### [bgred](index.js#L122)

Wrap a string with ansi codes to create a red background.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.bgred('some string'));
```

### [bgwhite](index.js#L137)

Wrap a string with ansi codes to create a white background.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.bgwhite('some string'));
```

### [bgyellow](index.js#L152)

Wrap a string with ansi codes to create a yellow background.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.bgyellow('some string'));
```

### [black](index.js#L167)

Wrap a string with ansi codes to create black text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.black('some string'));
```

### [blue](index.js#L182)

Wrap a string with ansi codes to create blue text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.blue('some string'));
```

### [bold](index.js#L197)

Wrap a string with ansi codes to create bold text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.bold('some string'));
```

### [cyan](index.js#L212)

Wrap a string with ansi codes to create cyan text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.cyan('some string'));
```

### [dim](index.js#L227)

Wrap a string with ansi codes to create dim text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.dim('some string'));
```

### [gray](index.js#L242)

Wrap a string with ansi codes to create gray text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.gray('some string'));
```

### [green](index.js#L257)

Wrap a string with ansi codes to create green text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.green('some string'));
```

### [grey](index.js#L272)

Wrap a string with ansi codes to create grey text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.grey('some string'));
```

### [hidden](index.js#L287)

Wrap a string with ansi codes to create hidden text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.hidden('some string'));
```

### [inverse](index.js#L302)

Wrap a string with ansi codes to create inverse text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.inverse('some string'));
```

### [italic](index.js#L317)

Wrap a string with ansi codes to create italic text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.italic('some string'));
```

### [magenta](index.js#L332)

Wrap a string with ansi codes to create magenta text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.magenta('some string'));
```

### [red](index.js#L347)

Wrap a string with ansi codes to create red text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.red('some string'));
```

### [reset](index.js#L362)

Wrap a string with ansi codes to reset ansi colors currently on the string.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.reset('some string'));
```

### [strikethrough](index.js#L377)

Wrap a string with ansi codes to add a strikethrough to the text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.strikethrough('some string'));
```

### [underline](index.js#L392)

Wrap a string with ansi codes to underline the text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.underline('some string'));
```

### [white](index.js#L407)

Wrap a string with ansi codes to create white text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.white('some string'));
```

### [yellow](index.js#L422)

Wrap a string with ansi codes to create yellow text.

**Params**

* `str` **{String}**: String to wrap with ansi codes.
* `returns` **{String}**: Wrapped string

**Example**

```js
console.log(colors.yellow('some string'));
```

## Related projects

* [ansi-bgblack](https://www.npmjs.com/package/ansi-bgblack): The color bgblack, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-bgblack)
* [ansi-bgblue](https://www.npmjs.com/package/ansi-bgblue): The color bgblue, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-bgblue)
* [ansi-bgcyan](https://www.npmjs.com/package/ansi-bgcyan): The color bgcyan, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-bgcyan)
* [ansi-bggreen](https://www.npmjs.com/package/ansi-bggreen): The color bggreen, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-bggreen)
* [ansi-bgmagenta](https://www.npmjs.com/package/ansi-bgmagenta): The color bgmagenta, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-bgmagenta)
* [ansi-bgred](https://www.npmjs.com/package/ansi-bgred): The color bgred, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-bgred)
* [ansi-bgwhite](https://www.npmjs.com/package/ansi-bgwhite): The color bgwhite, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-bgwhite)
* [ansi-bgyellow](https://www.npmjs.com/package/ansi-bgyellow): The color bgyellow, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-bgyellow)
* [ansi-black](https://www.npmjs.com/package/ansi-black): The color black, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-black)
* [ansi-blue](https://www.npmjs.com/package/ansi-blue): The color blue, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-blue)
* [ansi-bold](https://www.npmjs.com/package/ansi-bold): The color bold, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-bold)
* [ansi-cyan](https://www.npmjs.com/package/ansi-cyan): The color cyan, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-cyan)
* [ansi-dim](https://www.npmjs.com/package/ansi-dim): The color dim, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-dim)
* [ansi-gray](https://www.npmjs.com/package/ansi-gray): The color gray, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-gray)
* [ansi-green](https://www.npmjs.com/package/ansi-green): The color green, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-green)
* [ansi-grey](https://www.npmjs.com/package/ansi-grey): The color grey, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-grey)
* [ansi-hidden](https://www.npmjs.com/package/ansi-hidden): The color hidden, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-hidden)
* [ansi-inverse](https://www.npmjs.com/package/ansi-inverse): The color inverse, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-inverse)
* [ansi-italic](https://www.npmjs.com/package/ansi-italic): The color italic, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-italic)
* [ansi-magenta](https://www.npmjs.com/package/ansi-magenta): The color magenta, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-magenta)
* [ansi-red](https://www.npmjs.com/package/ansi-red): The color red, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-red)
* [ansi-reset](https://www.npmjs.com/package/ansi-reset): The color reset, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-reset)
* [ansi-strikethrough](https://www.npmjs.com/package/ansi-strikethrough): The color strikethrough, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-strikethrough)
* [ansi-underline](https://www.npmjs.com/package/ansi-underline): The color underline, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-underline)
* [ansi-white](https://www.npmjs.com/package/ansi-white): The color white, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-white)
* [ansi-yellow](https://www.npmjs.com/package/ansi-yellow): The color yellow, in ansi. | [homepage](https://github.com/jonschlinkert/ansi-yellow)

## Running tests

Install dev dependencies:

```sh
$ npm i -d && npm test
```

## Contributing

Pull requests and stars are always welcome. For bugs and feature requests, [please create an issue](https://github.com/doowb/ansi-colors/issues/new).

## Author

**Brian Woodward**

+ [github/doowb](https://github.com/doowb)
+ [twitter/doowb](http://twitter.com/doowb)

## License

Copyright © 2015 Brian Woodward
Released under the MIT license.

***

_This file was generated by [verb-cli](https://github.com/assemble/verb-cli) on November 30, 2015._