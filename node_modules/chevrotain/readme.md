[![Join the chat at https://gitter.im/chevrotain-parser/Lobby](https://badges.gitter.im/chevrotain-parser/Lobby.svg)](https://gitter.im/chevrotain-parser/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![npm](https://img.shields.io/npm/v/chevrotain.svg)](https://www.npmjs.com/package/chevrotain)
[![Build Status](https://travis-ci.org/SAP/chevrotain.svg?branch=master)](https://travis-ci.org/SAP/chevrotain)
[![Coverage Status](https://coveralls.io/repos/SAP/chevrotain/badge.svg?branch=master)](https://coveralls.io/r/SAP/chevrotain?branch=master)
[![Dependency status](https://img.shields.io/david/SAP/chevrotain.svg)](https://david-dm.org/SAP/chevrotain)
[![styled with prettier](https://img.shields.io/badge/styled_with-prettier-ff69b4.svg)](https://github.com/prettier/prettier)
[![Greenkeeper badge](https://badges.greenkeeper.io/SAP/chevrotain.svg)](https://greenkeeper.io/)

# Chevrotain

## Introduction

Chevrotain is a [**blazing fast**][benchmark] and [**feature rich**](http://sap.github.io/chevrotain/docs/features/blazing_fast.html) **Parser Building Toolkit** for **JavaScript**.
It can be used to build parsers/compilers/interpreters for various use cases ranging from simple configuration files,
to full fledged programing languages.

A more in depth description of Chevrotain can be found in this great article on: [Parsing in JavaScript: Tools and Libraries](https://tomassetti.me/parsing-in-javascript/#chevrotain).

It is important to note that Chevrotain is **NOT** a parser generator. It solves the same kind of problems as a parser generator, just **without any code generation**. Chevrotain Grammars are pure code which can be created/debugged/edited
as any other pure code without requiring any new tools or processes.

## TLDR

-   [**Online Playground**](https://sap.github.io/chevrotain/playground/)
-   **[Getting Started Tutorial](https://sap.github.io/chevrotain/docs/tutorial/step0_introduction.html)**
-   [**Performance benchmark**][benchmark]

## Installation

-   **npm**: `npm install chevrotain`
-   **Browser**:
    The npm package contains Chevrotain as concatenated and minified files ready for use in a browser.
    These can also be accessed directly via [UNPKG](https://unpkg.com/) in a script tag.
    -   Latest:
        -   `https://unpkg.com/chevrotain/lib/chevrotain.js`
        -   `https://unpkg.com/chevrotain/lib/chevrotain.min.js`
    -   Explicit version number:
        -   `https://unpkg.com/chevrotain@4.2.0/lib/chevrotain.js`
        -   `https://unpkg.com/chevrotain@4.2.0/lib/chevrotain.min.js`

## Documentation & Resources

-   **[Getting Started Tutorial](https://sap.github.io/chevrotain/docs/tutorial/step1_lexing.html)**.

-   **[Sample Grammars](https://github.com/SAP/chevrotain/blob/master/examples/grammars)**.

-   **[FAQ](https://sap.github.io/chevrotain/docs/FAQ.html).**

-   **[Other Examples](https://github.com/SAP/chevrotain/blob/master/examples)**.

-   **[HTML API docs](https://sap.github.io/chevrotain/documentation).**

    -   [The Parsing DSL Docs](https://sap.github.io/chevrotain/documentation/4_2_0/classes/parser.html#at_least_one).

## Dependencies

There is a single dependency to [regexp-to-ast](https://github.com/bd82/regexp-to-ast) library.
This dependency is included in the bundled artifacts, for ease of consumption in browsers.

## Compatibility

Chevrotain runs on any modern JavaScript ES5.1 runtime.
That includes any modern nodejs version, modern browsers and even IE11.

-   Uses [UMD](https://github.com/umdjs/umd) to work with common module loaders (browser global / amd / commonjs).

## Contributions

Contributions are **greatly** appreciated.
See [CONTRIBUTING.md](./CONTRIBUTING.md) for details.

## Where used

Some interesting samples:

-   [JHipster Domain Language][sample_jhipster]
-   [Metabase BI expression parser][sample_metabase].
-   [Eve Programing Language][sample_eve].
-   [BioModelAnalyzer's ChatBot parser][sample_biomodel].
-   [Bombadil Toml Parser][sample_bombadil]
-   [BrightScript Parser][sample_bright]

[benchmark]: https://sap.github.io/chevrotain/performance/
[sample_metabase]: https://github.com/metabase/metabase/blob/136dfb17954f4e4302b3bf2fee99ff7b7b12fd7c/frontend/src/metabase/lib/expressions/parser.js
[sample_jhipster]: https://github.com/jhipster/jhipster-core/blob/master/lib/dsl/jdl_parser.js
[sample_eve]: https://github.com/witheve/Eve/blob/master/src/parser/parser.ts
[sample_biomodel]: https://github.com/Microsoft/BioModelAnalyzer/blob/master/ChatBot/src/NLParser/NLParser.ts
[sample_bombadil]: https://github.com/sgarciac/bombadil/blob/master/src/parser.ts
[sample_bright]: https://github.com/RokuRoad/bright/blob/master/src/Parser.ts
[languages]: https://github.com/SAP/chevrotain/tree/master/examples/implementation_languages
[backtracking]: https://github.com/SAP/chevrotain/blob/master/examples/parser/backtracking/backtracking.js
[custom_apis]: https://sap.github.io/chevrotain/docs/guide/custom_apis.html
