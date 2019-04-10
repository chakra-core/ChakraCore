## Sort Package.json

[![Build Status](https://travis-ci.org/keithamus/sort-package-json.svg)](https://travis-ci.org/keithamus/sort-package-json)

Pass it a JSON string, it'll return a new JSON string, sorted by the keys typically found in a package.json

Pass it an object, it'll return an object sorted by the keys typically found in a package.json

```js
JSON.stringify(sortPackageJson({
  dependencies: {},
  version: '1.0.0',
  keywords: ['thing'],
  name: 'foo',
}), null, 2)
/* string:
{
  "name": "foo",
  "version": "1.0.0",
  "keywords": [
    "thing"
  ],
  "dependencies": {}
}
*/
```

### Install Permanently

#### API
```sh
npm install --save sort-package-json
```

#### CLI
```sh
npm install --global sort-package-json
```

### CLI Usage:

```bash
$ cd my-project
$ cat package.json
{
  "dependencies": {},
  "version": "1.0.0",
  "keywords": [
    "thing"
  ],
  "name": "foo"
}
$ npm i -g sort-package-json
$ sort-package-json
/my-project/package.json is sorted!
$ cat package.json
{
  "name": "foo",
  "version": "1.0.0",
  "keywords": [
    "thing"
  ],
  "dependencies": {}
}
```

`sort-package-json` also supports arguments - so you can give it a bunch of package.json file(s) to sort.

```bash
$ sort-package-json packages/*/package.json

$ sort-package-json my-package/package.json other-package/package.json
```

### Run (with npm@5.2+)

```sh
npx sort-package-json
```

### Visual Studio Code Extension

<https://github.com/unional/vscode-sort-package-json>

### Supported Libraries

- [bowserify](http://browserify.org/)
- [babel](https://babeljs.io/)
- [eslint](https://eslint.org/)
- [jest](https://jestjs.io/)
- [xo](https://github.com/xojs/xo)
- [prettier](https://prettier.io/)

### PFAQ: Potential Frequently Asked Questions

#### How does it sort?

It sorts using [`sort-object-keys`](http://github.com/keithamus/sort-object-keys). It sorts using the well-known keys of a package.json. For the full list it's just easier to [read the code](./index.js). It sorts sub-keys too - sometimes by a well-known order, other times alphabetically. The initial order was derived from the [package.json docs](https://docs.npmjs.com/files/package.json) with a few extras added for good measure.

#### It doesn't sort X?

Cool. Send a PR! It might get denied if it is a specific vendor key of an unpopular project (e.g. `"my-super-unknown-project"`). We sort keys like "browserify" because it is a project with millions of users. If your project has, say, over 100 users, then we'll add it. Sound fair?

#### Isn't this just like Project X?

Could be. I wanted this one because at the time of writing, nothing is:

 - Zero config
 - Able to be used in a library
 - Quiet (i.e. not spitting out annoying log messages, when used in a library mode)


#### What?! Why would you want to do this?!

Well, it's nice to have the keys of a package.json in a well sorted order. Almost everyone would agree having "name" at the top of a package.json is sensible (rather than sorted alphabetically or somewhere silly like the bottom), so why not the rest of the package.json?
