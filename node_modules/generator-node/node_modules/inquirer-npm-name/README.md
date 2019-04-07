# inquirer-npm-name [![NPM version][npm-image]][npm-url] [![Build Status][travis-image]][travis-url] [![Dependency Status][daviddm-image]][daviddm-url] [![Coverage percentage][coveralls-image]][coveralls-url]

Helper function using [inquirer](https://github.com/SBoudrias/Inquirer.js) to
validate a value provided in a prompt does not exist as a npm package.

The supplied value must be a valid package name (as per
[validate-npm-package-name]); otherwise, the user will again be prompted to
enter a name.

If the value is already used as a npm package, then the users will be prompted
and asked if they want to choose another one. If so, we'll recurse through the
same validation process until we have a name that is unused on the npm registry.
This is a helper to catch naming issue in advance, it is not a validation rule
as the user can always decide to continue with the same name.

## Install

```sh
$ npm install --save inquirer-npm-name
```

## Usage

```js
var inquirer = require('inquirer');
var askName = require('inquirer-npm-name');

askName(
  {
    name: 'name',
    message: 'Some Module Name' // Default: 'Module Name'
  },
  inquirer
).then(function(answer) {
  console.log(answer.name);
});

// Equivalent to {name: 'name'}
askName('name', inquirer).then(function(answer) {
  console.log(answer.name);
});
```

Inside a **Yeoman Generator** you'd call it this way:

```js
var generators = require('yeoman-generator');
var inquirer = require('inquirer');
var askName = require('inquirer-npm-name');

module.exports = generators.Base.extend({
  prompting: function() {
    return askName(
      {
        name: 'name',
        message: 'Module Name'
      },
      this
    ).then(function(name) {
      console.log(name);
    });
  }
});
```

`askName` takes 2 parameters:

1. `prompt` an [Inquirer prompt configuration](https://github.com/SBoudrias/Inquirer.js#question)
   or just a string to serve as name.
2. `inquirer` or any object with a `obj.prompt()` method.

**Returns:** A `Promise` resolved with the answer object.

## License

MIT © [Simon Boudrias](http://twitter.com/vaxilart)

[validate-npm-package-name]: https://npmjs.org/package/validate-npm-package-name
[npm-image]: https://badge.fury.io/js/inquirer-npm-name.svg
[npm-url]: https://npmjs.org/package/inquirer-npm-name
[travis-image]: https://travis-ci.org/SBoudrias/inquirer-npm-name.svg?branch=master
[travis-url]: https://travis-ci.org/SBoudrias/inquirer-npm-name
[daviddm-image]: https://david-dm.org/SBoudrias/inquirer-npm-name.svg?theme=shields.io
[daviddm-url]: https://david-dm.org/SBoudrias/inquirer-npm-name
[coveralls-image]: https://coveralls.io/repos/SBoudrias/inquirer-npm-name/badge.svg
[coveralls-url]: https://coveralls.io/r/SBoudrias/inquirer-npm-name
