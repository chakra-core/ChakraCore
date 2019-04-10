# broccoli-clean-css

[![NPM version](https://img.shields.io/npm/v/broccoli-clean-css.svg)](https://www.npmjs.com/package/broccoli-clean-css)
[![Build Status](https://travis-ci.org/shinnn/broccoli-clean-css.svg?branch=master)](https://travis-ci.org/shinnn/broccoli-clean-css)
[![Build status](https://ci.appveyor.com/api/projects/status/hxys0gltb6qpj0gm?svg=true)](https://ci.appveyor.com/project/ShinnosukeWatanabe/broccoli-clean-css)
[![Coverage Status](https://img.shields.io/coveralls/shinnn/broccoli-clean-css.svg)](https://coveralls.io/r/shinnn/broccoli-clean-css)
[![Dependency Status](https://david-dm.org/shinnn/broccoli-clean-css.svg)](https://david-dm.org/shinnn/broccoli-clean-css)
[![devDependency Status](https://david-dm.org/shinnn/broccoli-clean-css/dev-status.svg)](https://david-dm.org/shinnn/broccoli-clean-css#info=devDependencies)

[clean-css](https://github.com/jakubpawlowicz/clean-css) plugin for [Broccoli](https://github.com/broccolijs/broccoli)

```css
a {
  color: #FF0000;
}

a {
  border-radius: 4px 4px 4px 4px;
}
```

↓

```css
a{color:red;border-radius:4px}
```

## Installation

[Use npm.](https://docs.npmjs.com/cli/install)

```
npm install --save-dev broccoli-clean-css
```

## API

```javascript
const cleanCSS = require('broccoli-clean-css');
```

### cleanCSS(*tree* [, *options*])

*tree*: `String` or `Object` (broccoli tree)  
*options*: `Object` (directly passed to [clean-css options](https://github.com/jakubpawlowicz/clean-css#how-to-use-clean-css-api))  
Return: `Function`

Note that `relativeTo` option is relative to the source tree by default.

```javascript
//Brocfile.js
const cleanCSS = require('broccoli-clean-css');

module.exports = cleanCSS('styles');
```

## License

Copyright (c) 2014 - 2015 [Shinnosuke Watanabe](https://github.com/shinnn)

Licensed under [the MIT License](./LICENSE).
