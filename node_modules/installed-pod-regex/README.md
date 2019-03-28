# installed-pod-regex

[![NPM version](https://img.shields.io/npm/v/installed-pod-regex.svg)](https://www.npmjs.com/package/installed-pod-regex)
[![Build Status](https://travis-ci.org/shinnn/installed-pod-regex.svg?branch=master)](https://travis-ci.org/shinnn/installed-pod-regex)
[![Coverage Status](https://img.shields.io/coveralls/shinnn/installed-pod-regex.svg)](https://coveralls.io/r/shinnn/installed-pod-regex)
[![devDependency Status](https://david-dm.org/shinnn/installed-pod-regex/dev-status.svg)](https://david-dm.org/shinnn/installed-pod-regex#info=devDependencies)

Create a [regular expression](http://www.ecma-international.org/ecma-262/5.1/#sec-15.10) object that matches the newly installed [Pod](https://cocoapods.org/) list generated with [CocoaPods installation commands](https://guides.cocoapods.org/terminal/commands.html#group_installation)

```javascript
const stdout = `
Using colored 1.2
Installing rouge 1.10.1
Installing xcjobs 0.2.2 (was 0.1.2)
`;

stdout.match(installedPodRegex());
//=> ['Installing rouge 1.10.1', 'Installing xcjobs 0.2.2 (was 0.1.2)']
```

## Installation

### Package managers

#### [npm](https://www.npmjs.com/)

```
npm install installed-pod-regex
```

#### [bower](http://bower.io/)

```
bower install installed-pod-regex
```

### Standalone

[Download the script file directly.](https://raw.githubusercontent.com/shinnn/installed-pod-regex/master/browser.js)

## API

### installedPodRegex()

Return: [`RegExp`](https://developer.mozilla.org/docs/Web/JavaScript/Reference/Global_Objects/RegExp) instance with `g` and `m` flags

```javascript
const stdout = `
Installing rouge 1.10.1
Installing xcjobs 0.2.2 (was 0.1.2)
`;

const regex = installedPodRegex();

regex.exec(stdout);
//=> ['Installing rouge 1.10.1', 'rouge', '1.10.1', undefined]

// The last item of matching result would be a previous version (if available)
regex.exec(stdout);
//=> ['Installing xcjobs 0.2.2 (was 0.1.2)', 'xcjobs', '0.2.2', '0.1.2']

regex.exec(stdout);
//=> null
```

## License

[The Unlicense](./LICENSE)
