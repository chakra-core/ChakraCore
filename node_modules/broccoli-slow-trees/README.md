# broccoli-slow-trees

Prints the slowest nodes from a broccoli build.

## Installation

```sh
npm install --save broccoli-slow-trees
```

This package requires a Broccoli 1.0.x builder.

## Usage

```js
var broccoli = require('broccoli');
var printSlowNodes = require('broccoli-slow-trees');

var builder = new broccoli.Builder(outputNode);

builder.build()
  .then(function() {
    printSlowNodes(builder.outputNodeWrapper);
  });
```
