# @feathersjs/tools

[![Greenkeeper badge](https://badges.greenkeeper.io/feathersjs/tools.svg)](https://greenkeeper.io/)

[![Build Status](https://travis-ci.org/feathersjs/tools.png?branch=master)](https://travis-ci.org/feathersjs/tools)
[![Test Coverage](https://api.codeclimate.com/v1/badges/dcdc3acce49350a829d4/test_coverage)](https://codeclimate.com/github/feathersjs/tools/test_coverage)
[![Dependency Status](https://img.shields.io/david/feathersjs/tools.svg?style=flat-square)](https://david-dm.org/feathersjs/tools)
[![Download Status](https://img.shields.io/npm/dm/@feathersjs/tools.svg?style=flat-square)](https://www.npmjs.com/package/@feathersjs/tools)

> Codemods and other generator and repository  management tools

## Installation

```
npm install @feathersjs/tools --save
```

## Local exports

- `upgrade` contains the functionality to update a current Feathers application or plugin to version 3 (including rewriting all module `require`s to use the `@feathersjs` namespace)
- `transform` contains [JSCodeshift](https://github.com/facebook/jscodeshift/) utilities used mainly by the generator.

## Global tools

When `@feathersjs/tools` is installed globally, `convert-repository` is an internal tool that updates the old Feathers plugin infrastructure that is using Babel to the new one without (see https://github.com/feathersjs/feathers/issues/608 for why and how).

## License

Copyright (c) 2017

Licensed under the [MIT license](LICENSE).
