# Node Generator [![Build Status](https://secure.travis-ci.org/yeoman/generator-node.svg?branch=master)](https://travis-ci.org/yeoman/generator-node) [![Gitter](https://img.shields.io/badge/Gitter-Join_the_Yeoman_chat_%E2%86%92-00d06f.svg)](https://gitter.im/yeoman/yeoman) [![OpenCollective](https://opencollective.com/yeoman/backers/badge.svg)](https://opencollective.com/yeoman#support)

`generator-node` creates a base template to start a new Node.js module.

It is also easily composed into your own generators so you can only target your efforts at your generator's specific features.


## Install

```
$ npm install --global generator-node
```


## Usage

```
$ yo node
```

*Note that this template will generate files in the current directory, so be sure to change to a new directory first if you don't want to overwrite existing files.*

That'll generate a project with all the common tools setup. This includes:

- Filled `package.json` file
- [jest](https://facebook.github.io/jest/) unit test and code coverage (optionally tracked on [Coveralls](https://coveralls.io/))
- [ESLint](http://eslint.org/) linting and code style checking
- [Travis CI](https://travis-ci.org/) continuous integration (optional)
- [License](https://spdx.org/licenses/)


### Running tests

Once the project is scaffolded, inside the project folder run:

```
$ npm test
```

You can also directly use jest to run test on single files:

```
$ npm -g install jest-cli
$ jest --watch
```


### Publishing your code

Once your tests are passing (ideally with a Travis CI green run), you might be ready to publish your code to npm. We recommend you using [npm version](https://docs.npmjs.com/cli/version) to tag release correctly.

```
$ npm version major
$ git push --follow-tags
# ATTENTION: There is no turning back here.
$ npm publish
```


## Extend this generator

First of all, make sure you're comfortable with [Yeoman composability](http://yeoman.io/authoring/composability.html) feature. Then in your own generator:

```js
var Generator = require('yeoman-generator');

module.exports = class extends Generator({
  default() {
    this.composeWith(require.resolve('generator-node/generators/app'), {
      /* provide the options you want */
    });
  }
});
```


### Options

Here's a list of our supported options:

- `boilerplate` (Boolean, default true) include or not the boilerplate files (`lib/index.js`, `test/index.js`).
- `cli` (Boolean, default false) include or not a `lib/cli.js` file.
- `editorconfig` (Boolean, default true) include or not a `.editorconfig` file.
- `git` (Boolean, default true) include or not the git files (`.gitattributes`, `.gitignore`).
- `license` (Boolean, default true) include or not a `LICENSE` file.
- `travis` (Boolean, default true) include or not a `.travis.yml` file.
- `githubAccount` (String) Account name for GitHub repo location.
- `readme` (String) content of the `README.md` file. Given this option, generator-node will still generate the title (with badges) and the license section.


### Sub generators

If you don't need all the features provided by the main generator, you can still use a limited set of features by composing with our sub generators directly.

Remember you can see the options of each sub generators by running `yo node:sub --help`.

- `node:boilerplate`
- `node:cli`
- `node:editorconfig`
- `node:eslint`
- `node:git`
- `node:readme`


## Backers
Love Yeoman work and community? Help us keep it alive by donating funds to cover project expenses! <br />
[[Become a backer](https://opencollective.com/yeoman#support)]

  <a href="https://opencollective.com/yeoman/backers/0/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/0/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/1/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/1/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/2/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/2/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/3/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/3/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/4/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/4/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/5/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/5/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/6/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/6/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/7/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/7/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/8/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/8/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/9/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/9/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/10/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/10/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/11/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/11/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/12/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/12/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/13/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/13/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/14/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/14/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/15/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/15/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/16/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/16/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/17/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/17/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/18/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/18/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/19/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/19/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/20/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/20/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/21/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/21/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/22/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/22/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/23/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/23/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/24/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/24/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/25/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/25/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/26/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/26/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/27/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/27/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/28/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/28/avatar">
  </a>
  <a href="https://opencollective.com/yeoman/backers/29/website" target="_blank">
    <img src="https://opencollective.com/yeoman/backers/29/avatar">
  </a>

## License

MIT © Yeoman team (http://yeoman.io)
