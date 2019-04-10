Broccoli concatenator that generates & propagates sourcemaps
-------------------------------------------------

[![Build Status](https://travis-ci.org/ef4/broccoli-sourcemap-concat.svg?branch=master)](https://travis-ci.org/ef4/broccoli-sourcemap-concat)
[![Build status](https://ci.appveyor.com/api/projects/status/bpxeh0we50eod6xw/branch/master?svg=true)](https://ci.appveyor.com/project/embercli/broccoli-sourcemap-concat/branch/master)

This filter is designed to be fast & good enough. It can generates
source maps substantially faster than you'll get via
mozilla/source-map, because it's special-cased for straight
line-to-line contenation.

It discovers input sourcemaps in relative URLs, including data URIs.


### Usage

```js
var node = concat(node);
```

#### Advanced Usage

```js
var node = concat(node, {
  outputFile: '/output.js',
  header: ";(function() {",
  headerFiles: ['loader.js'],
  inputFiles: ['**/*'],
  footerFiles: ['auto-start.js'],
  footer: "}());",
  sourceMapConfig: { enabled: true },
  allowNone: false | true // defaults to false, and will error if trying to concat but no files are found.
});
```

The structure of `output.js` will be as follows:

```
// - header
// - ordered content of the files in headerFiles
// - un-ordered content of files matched by inputFiles, but not in headerFiles or footerFiles
// - ordered content of the files in footerFiles
// - footer
```

#### Debug Usage

*note: this is intended for debugging purposes only, and will most likely negatively affect your build performace is left enabled*

Setting the environment variable `CONCAT_STATS=true` will result a summary of
each concatention being output to `process.cwd() + 'concat-stats-for/*.json'`

Each file within that directory represents a different contenation, and will contain:

* outputFile – the output file that was created
* sizes – a summary of each input file, and the associated pre-minified pre-gziped byte size.

Want more details? like uglified or compressed sizes? (or have more ideas) go checkout: https://github.com/stefanpenner/broccoli-concat-analyser

##### Example:

concat-stats-for/<id>-file.json
```json
{
  "outputFile": "path/to/output/File",
  "sizes": {
    "a.js": 5,
    "b.js": 10,
  }
}
```

other files:

* concat-stats-for/<id>-file/a.js
* concat-stats-for/<id>-file/b.js
