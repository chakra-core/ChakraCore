# question-cache [![NPM version](https://img.shields.io/npm/v/question-cache.svg?style=flat)](https://www.npmjs.com/package/question-cache) [![NPM downloads](https://img.shields.io/npm/dm/question-cache.svg?style=flat)](https://npmjs.org/package/question-cache) [![Build Status](https://img.shields.io/travis/jonschlinkert/question-cache.svg?style=flat)](https://travis-ci.org/jonschlinkert/question-cache)

A wrapper around inquirer that makes it easy to create and selectively reuse questions.

## Install

Install with [npm](https://www.npmjs.com/):

```sh
$ npm install --save question-cache
```

## Example

```js
'use strict';

var Store = require('data-store');
var hints = new Store('example-hints-store');
var questions = require('question-cache');

questions()
  .use(function() {
    this.on('ask', function(val, key, question) {
      question.default = hints.get(key);
    });
    this.on('answer', function(val, key) {
      hints.set(key, val);
    });
  })
  .set('first', 'What is your first name?')
  .set('last', 'What is your last name?')
  .set('foo', 'What is foo?', {
    when: function() {
      // console.log(arguments)
    }
  })
  .ask(function(err, answers) {
    console.log(answers);
  });
```

**Screen capture**

<img width="669" alt="screen shot 2015-07-13 at 8 05 20 am" src="https://cloud.githubusercontent.com/assets/383994/8649221/00ecc908-2936-11e5-88d2-b1ab75a53ba0.png">

See the [working examples](./examples).

## Basic Usage

See the [working examples](./examples).

```js
var questions = require('question-cache')();

// question type "input" is used by default
questions
  .set('name', 'What is your name?')
  .ask('name', function (err, answers) {
    console.log(answers);
  });
```

**[inquirer2](https://github.com/jonschlinkert/inquirer2)**

You may optionally pass your own instance of inquirer to the constructor:

```js
// on the options
var questions = require('question-cache');
var questions = new Questions({
  inquirer: require('inquirer2')
});

// or if inquirer is the only thing passed
var questions = new Questions(require('inquirer2'));
```

## Getting started

question-cache is a wrapper around [inquirer2](https://github.com/jonschlinkert/inquirer2). If you have any issues related to the interface (like scrolling, colors, styling, etc), then please create an issue on the [inquirer2](https://github.com/jonschlinkert/inquirer2) project.

**Asking questions**

The simplest way to ask a question is by passing a string and a callback:

```js
questions.ask('name', function (err, answers) {
  console.log(answers);
});
```

**Ask all cached questions**

```js
questions.ask(function (err, answers) {
  console.log(answers);
});
```

## API

### [Questions](index.js#L27)

Create an instance of `Questions` with the given `options`.

**Params**

* `options` **{Object}**: question cache options

**Example**

```js
var Questions = new Questions(options);
```

### [.set](index.js#L88)

Calls [addQuestion](#addQuestion), with the only difference being that `.set` returns the `questions` instance and `.addQuestion` returns the question object. So use `.set` if you want to chain questions, or `.addQuestion` if you need the created question object.

**Params**

* `name` **{Object|String}**: Question name, message (string), or question/options object.
* `value` **{Object|String}**: Question message (string), or question/options object.
* `options` **{Object|String}**: Question/options object.

**Example**

```js
questions
  .set('drink', 'What is your favorite beverage?')
  .set('color', 'What is your favorite color?')
  .set('season', 'What is your favorite season?');

// or
questions.set('drink', {
  type: 'input',
  message: 'What is your favorite beverage?'
});

// or
questions.set({
  name: 'drink'
  type: 'input',
  message: 'What is your favorite beverage?'
});
```

### [.addQuestion](index.js#L120)

Add a question to be asked at a later point. Creates an instance of [Question](#question), so any `Question` options or settings may be used. Also, the default `type` is `input` if not defined by the user.

**Params**

* `name` **{Object|String}**: Question name, message (string), or question/options object.
* `value` **{Object|String}**: Question message (string), or question/options object.
* `options` **{Object|String}**: Question/options object.

**Example**

```js
questions.addQuestion('drink', 'What is your favorite beverage?');

// or
questions.addQuestion('drink', {
  type: 'input',
  message: 'What is your favorite beverage?'
});

// or
questions.addQuestion({
  name: 'drink'
  type: 'input',
  message: 'What is your favorite beverage?'
});
```

### [.choices](index.js#L162)

Create a "choices" question from an array of values.

**Params**

* `key` **{String}**: Question key
* `msg` **{String}**: Question message
* `items` **{Array}**: Choice items
* `options` **{Object|Function}**: Question options or callback function
* `callback` **{Function}**: callback function

**Example**

```js
questions.choices('foo', ['a', 'b', 'c']);

// or
questions.choices('foo', {
  message: 'Favorite letter?',
  choices: ['a', 'b', 'c']
});
```

### [.list](index.js#L193)

Create a "list" question from an array of values.

**Params**

* `key` **{String}**: Question key
* `msg` **{String}**: Question message
* `list` **{Array}**: List items
* `queue` **{String|Array}**: Name or array of question names.
* `options` **{Object|Function}**: Question options or callback function
* `callback` **{Function}**: callback function

**Example**

```js
questions.list('foo', ['a', 'b', 'c']);

// or
questions.list('foo', {
  message: 'Favorite letter?',
  choices: ['a', 'b', 'c']
});
```

### [.rawlist](index.js#L224)

Create a "rawlist" question from an array of values.

**Params**

* `key` **{String}**: Question key
* `msg` **{String}**: Question message
* `list` **{Array}**: List items
* `queue` **{String|Array}**: Name or array of question names.
* `options` **{Object|Function}**: Question options or callback function
* `callback` **{Function}**: callback function

**Example**

```js
questions.rawlist('foo', ['a', 'b', 'c']);

// or
questions.rawlist('foo', {
  message: 'Favorite letter?',
  choices: ['a', 'b', 'c']
});
```

### [.expand](index.js#L255)

Create an "expand" question from an array of values.

**Params**

* `key` **{String}**: Question key
* `msg` **{String}**: Question message
* `list` **{Array}**: List items
* `queue` **{String|Array}**: Name or array of question names.
* `options` **{Object|Function}**: Question options or callback function
* `callback` **{Function}**: callback function

**Example**

```js
questions.expand('foo', ['a', 'b', 'c']);

// or
questions.expand('foo', {
  message: 'Favorite letter?',
  choices: ['a', 'b', 'c']
});
```

### [.confirm](index.js#L282)

Create a "choices" question from an array of values.

**Params**

* `queue` **{String|Array}**: Name or array of question names.
* `options` **{Object|Function}**: Question options or callback function
* `callback` **{Function}**: callback function

**Example**

```js
questions.choices('foo', ['a', 'b', 'c']);
// or
questions.choices('foo', {
  message: 'Favorite letter?',
  choices: ['a', 'b', 'c']
});
```

### [.get](index.js#L301)

Get question `name`, or group `name` if question is not found. You can also do a direct lookup using `quesions.cache['foo']`.

**Params**

* `name` **{String}**
* `returns` **{Object}**: Returns the question object.

**Example**

```js
var name = questions.get('name');
//=> question object
```

### [.has](index.js#L317)

Returns true if `questions.cache` or `questions.groups` has question `name`.

* `returns` **{String}**: The name of the question to check

**Example**

```js
var name = questions.has('name');
//=> true
```

### [.del](index.js#L342)

Delete the given question or any questions that have the given namespace using dot-notation.

* `returns` **{String}**: The name of the question to delete

**Example**

```js
questions.del('name');
questions.get('name');
//=> undefined

// using dot-notation
questions.del('author');
questions.get('author.name');
//=> undefined
```

### [.clearAnswers](index.js#L360)

Clear all cached answers.

**Example**

```js
questions.clearAnswers();
```

### [.clearQuestions](index.js#L375)

Clear all questions from the cache.

**Example**

```js
questions.clearQuestions();
```

### [.clear](index.js#L390)

Clear all cached questions and answers.

**Example**

```js
questions.clear();
```

### [.ask](index.js#L409)

Ask one or more questions, with the given `options` and callback.

**Params**

* `queue` **{String|Array}**: Name or array of question names.
* `options` **{Object|Function}**: Question options or callback function
* `callback` **{Function}**: callback function

**Example**

```js
questions.ask(['name', 'description'], function(err, answers) {
  console.log(answers);
});
```

### [.normalize](index.js#L557)

Normalize the given value to return an array of question keys.

**Params**

* **{[type]}**: key
* `returns` **{[type]}**

## Dot notation

See the [working examples](./examples).

Qestions may be cached using object-path notatation (e.g. `a.b.c`).

**Example**

All of the following will be cached on the `name` object:

```js
questions
  .set('name.first', 'What is your first name?')
  .set('name.middle', 'What is your middle name?')
  .set('name.last', 'What is your last name?')
```

**Dot notation usage**

When cached using dot-notation, there are a few different ways questions that may be asked.

### Dot notation usage

#### ask one

Ask a single `name` question:

```js
questions.ask('name.first', function (err, answers) {
  console.log(answers);
});
```

#### ask all "name" questions

Ask all `name` questions, `first`, `middle` and `last`:

```js
questions.ask('name', function (err, answers) {
  console.log(answers);
});
```

#### array of questions

Ask specific questions on `name`:

```js
questions.ask(['name.first', 'name.last'], function (err, answers) {
  console.log(answers);
});
```

#### ask all questions

Ask specific questions on `name`:

```js
questions
  .set('name.first', {
    message: 'What is your first name?',
  })
  .set('name.last', {
    message: 'What is your last name?',
  })
  .set('foo', {
    message: 'Any thoughts about foo?',
  })

questions.ask(['name', 'foo'], function (err, answers) {
  console.log(answers);
});
```

#### nested questions

Ask one question at a time, based on feedback:

```js
questions.ask('name.first', function (err, answers) {
  console.log(answers);
  //=> {name: { first: 'Brian' }}

  questions.ask('name.last', function (err, answers) {
    console.log(answers);
    //=> {name: { last: 'Woodward' }}
  });
});
```

## More examples

### Mixture of dot notation and non-dot-notation

Given you have the following questions:

```js
questions
  .set('name.first', 'What is your first name?')
  .set('name.last', 'What is your last name?')
  .set('foo', 'Any thoughts about foo?')
  .set('bar', 'Any thoughts about bar?')
```

The following will ask questions: `name.first`, `name.last` and `foo`

```js
questions.ask(['name', 'foo'], function (err, answers) {
  console.log(answers);
});
```

## About

### Related projects

* [ask-for-github-auth](https://www.npmjs.com/package/ask-for-github-auth): Prompt a user for their github authentication credentials and save the results. | [homepage](https://github.com/doowb/ask-for-github-auth "Prompt a user for their github authentication credentials and save the results.")
* [ask-once](https://www.npmjs.com/package/ask-once): Only ask a question one time and store the answer. | [homepage](https://github.com/doowb/ask-once "Only ask a question one time and store the answer.")
* [generate](https://www.npmjs.com/package/generate): Command line tool and developer framework for scaffolding out new GitHub projects. Generate offers the… [more](https://github.com/generate/generate) | [homepage](https://github.com/generate/generate "Command line tool and developer framework for scaffolding out new GitHub projects. Generate offers the robustness and configurability of Yeoman, the expressiveness and simplicity of Slush, and more powerful flow control and composability than either.")
* [question-helper](https://www.npmjs.com/package/question-helper): Template helper that asks a question in the command line and resolves the template with… [more](https://github.com/doowb/question-helper) | [homepage](https://github.com/doowb/question-helper "Template helper that asks a question in the command line and resolves the template with the answer.")

### Contributing

Pull requests and stars are always welcome. For bugs and feature requests, [please create an issue](../../issues/new).

### Building docs

_(This document was generated by [verb-generate-readme](https://github.com/verbose/verb-generate-readme) (a [verb](https://github.com/verbose/verb) generator), please don't edit the readme directly. Any changes to the readme must be made in [.verb.md](.verb.md).)_

To generate the readme and API documentation with [verb](https://github.com/verbose/verb):

```sh
$ npm install -g verb verb-generate-readme && verb
```

### Running tests

Install dev dependencies:

```sh
$ npm install -d && npm test
```

### Author

**Jon Schlinkert**

* [github/jonschlinkert](https://github.com/jonschlinkert)
* [twitter/jonschlinkert](http://twitter.com/jonschlinkert)

### License

Copyright © 2016, [Jon Schlinkert](https://github.com/jonschlinkert).
Released under the [MIT license](https://github.com/jonschlinkert/question-cache/blob/master/LICENSE).

***

_This file was generated by [verb-generate-readme](https://github.com/verbose/verb-generate-readme), v0.1.30, on August 17, 2016._