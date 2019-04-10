
heimdall-fs-monitor
==============================================================================

[![npm](https://img.shields.io/npm/v/heimdalljs-fs-monitor.svg)](https://www.npmjs.com/package/heimdalljs-fs-monitor)

file system monitor plugin for [heimdalljs](https://github.com/heimdalljs/heimdalljs-lib)


Installation
------------------------------------------------------------------------------

```
npm install --save-dev heimdalljs-fs-monitor
```

Usage
------------------------------------------------------------------------------

```js
var FSMonitor = require('heimdalljs-fs-monitor');

// create a new file system monitor
var monitor = new FSMonitor();

// start monitoring
monitor.start();

// ... do some file system work ...
var fs = require('fs');
fs.readFileSync('package.json');
 
// stop monitoring
monitor.stop();

// read file system call stats
var heimdall = require('heimdalljs');
var stats = heimdall.statsFor('fs');
```


License
------------------------------------------------------------------------------
heimdall-fs-monitor is licensed under the [ISC License](https://opensource.org/licenses/isc-license.txt).
