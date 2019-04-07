[![Stories in Ready](https://badge.waffle.io/twigjs/twig.js.png?label=ready&title=Ready)](https://waffle.io/twigjs/twig.js)
[![Known Vulnerabilities](https://snyk.io/test/github/twigjs/twig.js/badge.svg)](https://snyk.io/test/github/twigjs/twig.js)
[![Build Status](https://secure.travis-ci.org/twigjs/twig.js.svg)](http://travis-ci.org/#!/twigjs/twig.js)
[![NPM version](https://badge.fury.io/js/twig.svg)](http://badge.fury.io/js/twig)
[![Gitter](https://badges.gitter.im/twigjs/twig.js.svg)](https://gitter.im/twigjs/twig.js?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

# About

<img align="right" width="120" height="120"
     title="Twig.js"
     src="https://user-images.githubusercontent.com/3282350/29336704-ab1be05c-81dc-11e7-92e5-cf11cca7b344.png">

Twig.js is a pure JavaScript implementation of the Twig PHP templating language
(<http://twig.sensiolabs.org/>)

The goal is to provide a library that is compatible with both browsers and server side JavaScript environments such as node.js.

Twig.js is currently a work in progress and supports a limited subset of the Twig templating language (with more coming).

### Docs

Documentation is available in the [twig.js wiki](https://github.com/twigjs/twig.js/wiki) on Github.

### Feature Support

For a list of supported tags/filters/functions/tests see the [Implementation Notes](https://github.com/twigjs/twig.js/wiki/Implementation-Notes) page on the wiki.

# Install

Download the latest twig.js release from github: https://github.com/twigjs/twig.js/releases or via NPM:

```bash
npm install twig --save
```

# Bower

A bower package is available from [philsbury](https://github.com/philsbury/twigjs-bower). Please direct any Bower support issues to that repo.

## Browser Usage

Include twig.js or twig.min.js in your page, then:

```js
var template = twig({
    data: 'The {{ baked_good }} is a lie.'
});

console.log(
    template.render({baked_good: 'cupcake'})
);
// outputs: "The cupcake is a lie."
```

## Webpack

A loader is available from [zimmo.be](https://github.com/zimmo-be/twig-loader).

## Node Usage (npm)

Tested on node >=6.0.

You can use twig in your app with

```js
var Twig = require('twig'), // Twig module
    twig = Twig.twig;       // Render function
```

### Usage without Express

If you don't want to use Express, you can render a template with the following method:

```js
import Twig from 'twig';
Twig.renderFile('./path/to/someFile.twig', {foo:'bar'}, (err, html) => {
  html; // compiled string
});
```

### Usage with Express

Twig is compatible with express 2 and 3. You can create an express app using the twig.js templating language by setting the view engine to twig.

### app.js

**Express 3**

```js
var Twig = require("twig"),
    express = require('express'),
    app = express();

// This section is optional and used to configure twig.
app.set("twig options", {
    allow_async: true, // Allow asynchronous compiling
    strict_variables: false
});

app.get('/', function(req, res){
  res.render('index.twig', {
    message : "Hello World"
  });
});

app.listen(9999);
```

## views/index.twig

```html
Message of the moment: <b>{{ message }}</b>
```

An [Express 2 Example](https://github.com/twigjs/twig.js/wiki/Express-2) is available on the wiki.

# Alternatives

- [Twing](https://github.com/ericmorand/twing)

# Contributing

If you have a change you want to make to twig.js, feel free to fork this repository and submit a pull request on Github. The source files are located in src/*.js.

twig.js is built by running `npm run build`

For more details on getting setup, see the [contributing page](https://github.com/twigjs/twig.js/wiki/Contributing) on the wiki.

## Tests

The twig.js tests are written in [Mocha][mocha] and can be invoked with `npm test`.

## License

Twig.js is available under a [BSD 2-Clause License][bsd-2], see the LICENSE file for more information.

## Acknowledgments

See the LICENSES.md file for copies of the referenced licenses.

1. The JavaScript Array fills in src/twig.fills.js are from <https://developer.mozilla.org/> and are available under the [MIT License][mit] or are [public domain][mdn-license].

2. The Date.format function in src/twig.lib.js is from <http://jpaq.org/> and used under a [MIT license][mit-jpaq].

3. The sprintf implementation in src/twig.lib.js used for the format filter is from <http://www.diveintojavascript.com/projects/javascript-sprintf> and used under a [BSD 3-Clause License][bsd-3].

4. The strip_tags implementation in src/twig.lib.js used for the striptags filter is from <http://phpjs.org/functions/strip_tags> and used under and [MIT License][mit-phpjs].

[mit-jpaq]:     http://jpaq.org/license/
[mit-phpjs]:    http://phpjs.org/pages/license/#MIT
[mit]:          http://www.opensource.org/licenses/mit-license.php
[mdn-license]:  https://developer.mozilla.org/Project:Copyrights

[bsd-2]:        http://www.opensource.org/licenses/BSD-2-Clause
[bsd-3]:        http://www.opensource.org/licenses/BSD-3-Clause
[cc-by-sa-2.5]: http://creativecommons.org/licenses/by-sa/2.5/ "Creative Commons Attribution-ShareAlike 2.5 License"

[mocha]:        http://mochajs.org/
[qunit]:        http://docs.jquery.com/QUnit
