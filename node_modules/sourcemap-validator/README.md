sourcemap-validator
===================

[![Build Status](https://travis-ci.org/ben-ng/sourcemap-validator.png)](https://travis-ci.org/ben-ng/sourcemap-validator)

Mapped all the things? Now validate all the maps.

## Usage

```js
validate(minifiedCode, [sourceMap], [sourceContent]);
```

 * `minifiedCode` is your minified code as a string
 * `sourceMap` is your sourcemap as a JSON string
    * Optional - If left empty, the inline sourcemap in `minifiedCode` will be used
 * `sourceContent` is a map to the raw source files
    * Optional - If left empty, the inline `sourceContent` in `sourceMap` will be used

## Examples

```js
var validate = require('sourcemap-validator')
  , fs = require('fs')
  , assert = require('assert')
  , min = fs.readFileSync('jquery.min.js', 'utf-8')
  , map = fs.readFileSync('jquery.min.map', 'utf-8');

assert.doesNotThrow(function () {
  validate(min, map, {'jquery.js': raw});
}, 'The sourcemap is not valid');
```

```js
var validate = require('sourcemap-validator')
  , fs = require('fs')
  , assert = require('assert')
  , min = fs.readFileSync('bundle.min.js', 'utf-8')
  , map = fs.readFileSync('bundle.min.map', 'utf-8');

// Browserify bundles have inline sourceContent in their maps
// so no need to pass a `sourceContent` object.
assert.doesNotThrow(function () {
  validate(min, map);
}, 'The sourcemap is not valid');
```

### validate(sourceContent, minifiedCode, mapJSON)

## Notes

The sourcemap spec isn't exactly very mature, so this module only aims to give you a Pretty Good™ idea of whether or not your sourcemap is correct.

### Quoted keys in object literals

If a sourcemap maps "literal" without the quotes to column 3, we will consider that valid.

**Example**
```js
var v = {
  literal: true
//^-- ok to map {name: literal, column: 3} here
};

var t = {
  "literal": true
//^-- ok to map {name: literal, column: 3} here, even though the token actually appears in column 4
};
```

See the discussion [here](https://github.com/mishoo/UglifyJS2/pull/303#issuecomment-27628362)

However, mapping something totally wrong like `"cookie"` to that index will throw an exception.

### Missing mappings

There is no way for the validator to know if you are missing mappings. It can only ensure that the ones you made are sensible. The validator will consider a sourcemap with zero mappings invalid as a sanity check, but if your map at least one sensisble mapping, it is a valid map.

## License
The MIT License (MIT)

Copyright (c) 2013 Ben Ng

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

