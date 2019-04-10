# prettier-eslint

Formats your JavaScript using [`prettier`][prettier] followed by [`eslint --fix`][eslint]

[![Build Status][build-badge]][build]
[![Code Coverage][coverage-badge]][coverage]
[![Dependencies][dependencyci-badge]][dependencyci]
[![version][version-badge]][package]
[![downloads][downloads-badge]][npm-stat]
[![MIT License][license-badge]][LICENSE]

[![All Contributors](https://img.shields.io/badge/all_contributors-13-orange.svg?style=flat-square)](#contributors)
[![PRs Welcome][prs-badge]][prs]
[![Donate][donate-badge]][donate]
[![Code of Conduct][coc-badge]][coc]
[![Roadmap][roadmap-badge]][roadmap]
[![Examples][examples-badge]][examples]

[![Watch on GitHub][github-watch-badge]][github-watch]
[![Star on GitHub][github-star-badge]][github-star]
[![Tweet][twitter-badge]][twitter]

## The problem

The [`fix`][fix] feature of [`eslint`][eslint] is pretty great and can auto-format/fix much of your code according to
your ESLint config. [`prettier`][prettier] is a more powerful automatic formatter. One of the nice things about prettier
is how opinionated it is. Unfortunately it's not opinionated enough and/or some opinions differ from my own. So after
prettier formats the code, I start getting linting errors.

## This solution

This formats your code via `prettier`, and then passes the result of that to `eslint --fix`. This way you can get the
benefits of `prettier`'s superior formatting capabilities, but also benefit from the configuration capabilities of
`eslint`.

## Installation

This module is distributed via [npm][npm] which is bundled with [node][node] and should be installed as one of your
project's `devDependencies`:

```
npm install --save-dev prettier-eslint
```

## Usage

### Example

```javascript
const format = require('prettier-eslint')

// notice, no semicolon in the original text
const sourceCode = 'const {foo} = bar'

const options = {
  text: sourceCode,
  eslintConfig: {
    parserOptions: {
      ecmaVersion: 7,
    },
    rules: {
      semi: ['error', 'never'],
    },
  },
  prettierOptions: {
    bracketSpacing: true,
  },
}

const formatted = format(options)

// notice no semicolon in the formatted text
formatted // const { foo } = bar
```

### options

#### text (String)

The source code to format.

#### filePath (?String)

The path of the file being formatted can be used to override `eslintConfig` (eslint will be used to find the relevant
config for the file).

#### eslintConfig (?Object)

The config to use for formatting with ESLint. Can be overridden with `filePath`.

#### prettierOptions (?Object)

The options to pass for formatting with `prettier`. If not provided, `prettier-eslint` will attempt to create the
options based on the `eslintConfig` (whether that's provided or derived via `filePath`). You can also provide _some_ of
the options and have the remaining options derived via your eslint config. This is useful for options like `parser`.

#### logLevel (?Enum: ['trace', 'debug', 'info', 'warn', 'error', 'silent'])

`prettier-eslint` does quite a bit of logging if you want it to. Pass this to set the amount of logs you want to see.
Default is `process.env.LOG_LEVEL || 'warn'`.

#### eslintPath (?String)

By default, `prettier-eslint` will try to find the relevant `eslint` (and `prettier`) module based on the `filePath`. If
it cannot find one, then it will use the version that `prettier-eslint` has installed locally. If you'd like to specify
a path to the `eslint` module you would like to have `prettier-eslint` use, then you can provide the full path to it
with the `eslintPath` option.

#### prettierPath (?String)

This is basically the same as `eslintPath` except for the `prettier` module.

#### prettierLast (?Boolean)

By default, `prettier-eslint` will run `prettier` first, then `eslint --fix`. This is great if you want to use `prettier`,
but override some of the styles you don't like using `eslint --fix`.

An alternative approach is to use different tools for different concerns. If you provide `pretterLast: true`, it will
run `eslint --fix` first, then `prettier`. This allows you to use `eslint` to look for bugs and/or bad practices, and use
`prettier` to enforce code style.

### throws

`prettier-eslint` will propagate errors when either `prettier` or `eslint` fails for one reason or another. In addition
to propagating the errors, it will also log a specific message indicating what it was doing at the time of the failure.

## Technical details

> Code ➡️ prettier ➡️ eslint --fix ➡️ Formatted Code ✨

### inferring prettierOptions via eslintConfig

The `eslintConfig` and `prettierOptions` can each be provided as an argument. If
the `eslintConfig` is not provided, then `prettier-eslint` will look for them
based on the `fileName` (if no `fileName` is provided then it uses
`process.cwd()`). Once `prettier-eslint` has found the `eslintConfig`, the
`prettierOptions` are inferred from the `eslintConfig`. If some of the
`prettierOptions` have already been provided, then `prettier-eslint` will only
infer the remaining options. This inference happens in `src/utils.js`.

**An important thing to note** about this inference is that it may not support
your specific eslint config. So you'll want to check `src/utils.js` to see how
the inference is done for each option (what rule(s) are referenced, etc.) and
[make a pull request][prs] if your configuration is supported.

**Defaults** if you have all of the relevant ESLint rules disabled (or have
ESLint disabled entirely via `/* eslint-disable */` then prettier options will
fall back to the defaults:

```javascript
{
  printWidth: 80,
  tabWidth: 2,
  parser: 'babylon',
  singleQuote: true,
  trailingComma: 'all',
  bracketSpacing: false,
}
```

I set these defaults based on my own preferences. If you'd like to see these
defaults changed, please [make a pull request][pr].

## Inspiration

- [`prettier`][prettier]
- [`eslint`][eslint]

## Other Solutions

None that I'm aware of. Feel free to file a PR if you know of any other solutions.

## Related

- [`prettier-eslint-cli`](https://github.com/prettier/prettier-eslint-cli) - Command Line Interface
- [`prettier-eslint-atom`](https://github.com/kentcdodds/prettier-eslint-atom) - Atom plugin
- [`prettier-eslint-vscode`](https://github.com/RobinMalfait/prettier-eslint-code) - Visual Studio Code plugin
- [`eslint-plugin-prettier`](https://github.com/not-an-aardvark/eslint-plugin-prettier) - ESLint plugin. While prettier-eslint uses `eslint --fix` to change the output of `prettier`, eslint-plugin-prettier keeps the `prettier` output as-is and integrates it with the regular ESLint workflow.
- [`prettier-eslint-webpack-plugin`](https://github.com/danielterwiel/prettier-eslint-webpack-plugin) - Prettier ESlint Webpack Plugin

## Contributors

Thanks goes to these people ([emoji key][emojis]):

<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
| [<img src="https://avatars.githubusercontent.com/u/1500684?v=3" width="100px;"/><br /><sub>Kent C. Dodds</sub>](https://kentcdodds.com)<br />[💻](https://github.com/prettier/prettier-eslint/commits?author=kentcdodds) [📖](https://github.com/prettier/prettier-eslint/commits?author=kentcdodds) 🚇 [⚠️](https://github.com/prettier/prettier-eslint/commits?author=kentcdodds) | [<img src="https://avatars.githubusercontent.com/u/5554486?v=3" width="100px;"/><br /><sub>Gyandeep Singh</sub>](http://gyandeeps.com)<br />👀 | [<img src="https://avatars.githubusercontent.com/u/682584?v=3" width="100px;"/><br /><sub>Igor Pnev</sub>](https://github.com/exdeniz)<br />[🐛](https://github.com/prettier/prettier-eslint/issues?q=author%3Aexdeniz) | [<img src="https://avatars.githubusercontent.com/u/813865?v=3" width="100px;"/><br /><sub>Benjamin Tan</sub>](https://demoneaux.github.io/)<br />💬 👀 | [<img src="https://avatars.githubusercontent.com/u/622118?v=3" width="100px;"/><br /><sub>Eric McCormick</sub>](https://ericmccormick.io)<br />[💻](https://github.com/prettier/prettier-eslint/commits?author=edm00se) [📖](https://github.com/prettier/prettier-eslint/commits?author=edm00se) [⚠️](https://github.com/prettier/prettier-eslint/commits?author=edm00se) | [<img src="https://avatars.githubusercontent.com/u/2142817?v=3" width="100px;"/><br /><sub>Simon Lydell</sub>](https://github.com/lydell)<br />[📖](https://github.com/prettier/prettier-eslint/commits?author=lydell) | [<img src="https://avatars0.githubusercontent.com/u/981957?v=3" width="100px;"/><br /><sub>Tom McKearney</sub>](https://github.com/tommck)<br />[📖](https://github.com/prettier/prettier-eslint/commits?author=tommck) 💡 |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| [<img src="https://avatars.githubusercontent.com/u/463105?v=3" width="100px;"/><br /><sub>Patrik Åkerstrand</sub>](https://github.com/PAkerstrand)<br />[💻](https://github.com/prettier/prettier-eslint/commits?author=PAkerstrand) | [<img src="https://avatars.githubusercontent.com/u/1560301?v=3" width="100px;"/><br /><sub>Lochlan Bunn</sub>](https://twitter.com/loklaan)<br />[💻](https://github.com/prettier/prettier-eslint/commits?author=loklaan) | [<img src="https://avatars.githubusercontent.com/u/25886902?v=3" width="100px;"/><br /><sub>Daniël Terwiel</sub>](https://github.com/danielterwiel)<br />🔌 🔧 | [<img src="https://avatars1.githubusercontent.com/u/1834413?v=3" width="100px;"/><br /><sub>Robin Malfait</sub>](https://robinmalfait.com)<br />🔧 | [<img src="https://avatars0.githubusercontent.com/u/8161781?v=3" width="100px;"/><br /><sub>Michael McDermott</sub>](http://mgmcdermott.com)<br />[💻](https://github.com/prettier/prettier-eslint/commits?author=mgmcdermott) | [<img src="https://avatars3.githubusercontent.com/u/292365?v=3" width="100px;"/><br /><sub>Adam Stankiewicz</sub>](http://sheerun.net)<br />[💻](https://github.com/prettier/prettier-eslint/commits?author=sheerun) |
<!-- ALL-CONTRIBUTORS-LIST:END -->

This project follows the [all-contributors][all-contributors] specification. Contributions of any kind welcome!

## LICENSE

MIT

[prettier]: https://github.com/jlongster/prettier
[eslint]: http://eslint.org/
[fix]: http://eslint.org/docs/user-guide/command-line-interface#fix
[npm]: https://www.npmjs.com/
[node]: https://nodejs.org
[build-badge]: https://img.shields.io/travis/prettier/prettier-eslint.svg?style=flat-square
[build]: https://travis-ci.org/prettier/prettier-eslint
[coverage-badge]: https://img.shields.io/codecov/c/github/prettier/prettier-eslint.svg?style=flat-square
[coverage]: https://codecov.io/github/prettier/prettier-eslint
[dependencyci-badge]: https://dependencyci.com/github/prettier/prettier-eslint/badge?style=flat-square
[dependencyci]: https://dependencyci.com/github/prettier/prettier-eslint
[version-badge]: https://img.shields.io/npm/v/prettier-eslint.svg?style=flat-square
[package]: https://www.npmjs.com/package/prettier-eslint
[downloads-badge]: https://img.shields.io/npm/dm/prettier-eslint.svg?style=flat-square
[npm-stat]: http://npm-stat.com/charts.html?package=prettier-eslint&from=2016-04-01
[license-badge]: https://img.shields.io/npm/l/prettier-eslint.svg?style=flat-square
[license]: https://github.com/prettier/prettier-eslint/blob/master/other/LICENSE
[prs-badge]: https://img.shields.io/badge/PRs-welcome-brightgreen.svg?style=flat-square
[prs]: http://makeapullrequest.com
[donate-badge]: https://img.shields.io/badge/$-support-green.svg?style=flat-square
[donate]: http://kcd.im/donate
[coc-badge]: https://img.shields.io/badge/code%20of-conduct-ff69b4.svg?style=flat-square
[coc]: https://github.com/prettier/prettier-eslint/blob/master/other/CODE_OF_CONDUCT.md
[roadmap-badge]: https://img.shields.io/badge/%F0%9F%93%94-roadmap-CD9523.svg?style=flat-square
[roadmap]: https://github.com/prettier/prettier-eslint/blob/master/other/ROADMAP.md
[examples-badge]: https://img.shields.io/badge/%F0%9F%92%A1-examples-8C8E93.svg?style=flat-square
[examples]: https://github.com/prettier/prettier-eslint/blob/master/other/EXAMPLES.md
[github-watch-badge]: https://img.shields.io/github/watchers/prettier/prettier-eslint.svg?style=social
[github-watch]: https://github.com/prettier/prettier-eslint/watchers
[github-star-badge]: https://img.shields.io/github/stars/prettier/prettier-eslint.svg?style=social
[github-star]: https://github.com/prettier/prettier-eslint/stargazers
[twitter]: https://twitter.com/intent/tweet?text=Check%20out%20prettier-eslint!%20https://github.com/prettier/prettier-eslint%20%F0%9F%91%8D
[twitter-badge]: https://img.shields.io/twitter/url/https/github.com/prettier/prettier-eslint.svg?style=social
[emojis]: https://github.com/kentcdodds/all-contributors#emoji-key
[all-contributors]: https://github.com/kentcdodds/all-contributors
