# deep-diff [![Build Status](https://travis-ci.org/flitbit/diff.png?branch=master)](https://travis-ci.org/flitbit/diff) 
[![NPM](https://nodei.co/npm/deep-diff.png?downloads=true&downloadRank=true&stars=true)](https://nodei.co/npm/deep-diff/)

**deep-diff** is a javascript/node.js module providing utility functions for determining the structural differences between objects and includes some utilities for applying differences across objects.

## Features

* Get the structural differences between two objects.
* Observe the structural differences between two objects.
* When structural differences represent change, apply change from one object to another.
* When structural differences represent change, selectively apply change from one object to another.

## ChangeLog

`0.3.8` - 2017-05-03
* reconciled recently introduced difference between `index.es.js` and `index.js`
* improved npm commands for more reliable contributions
* added a few notes to README regarding contributing.

`0.3.7` - 2017-05-01
* fixed issue #98 by merging @sberan's pull request #99 &mdash; better handling of property with `undefined` value existing on either operand. Unit tests supplied.

`0.3.6` - 2017-04-25 &mdash; Fixed, closed lingering issues:
* fixed #74 &mdash; comparing objects with longer cycles
* fixed #70 &mdash; was not properly detecting a deletion when a property on the operand (lhs) had a value of `undefined` and was _undefined_ on the comparand (rhs). :o).
* WARNING! [Still broken when importing in Typescript](https://github.com/flitbit/diff/issues/97).

`0.3.5` - 2017-04-23 &mdash; Rolled up recent fixes; patches:
* @stevemao &mdash; #79, #80: now testing latest version of node4
* @mortonfox &mdash; #85: referencing mocha's new home
* @tdebarochez &mdash; #90: fixed error when .toString not a function
* @thiamsantos &mdash; #92, #93: changed module system for improved es2015 modules. WARNING! [This PR broke importing `deep-diff` in Typescript as reported by @kgentes in #97](https://github.com/flitbit/diff/issues/97)

`0.3.4` - Typescript users, reference this version until #97 is fixed!

`0.3.3` - Thanks @SimenB: enabled npm script for release (alternate to the Makefile). Also linting as part of `npm test`. Thanks @joeldenning: Fixed issue #35; diffs of top level arrays now working.

`0.3.3` - Thanks @SimenB: enabled npm script for release (alternate to the Makefile). Also linting as part of `npm test`. Thanks @joeldenning: Fixed issue #35; diffs of top level arrays now working.

`0.3.2` - Resolves #46; support more robust filters by including `lhs` and `rhs` in the filter callback. By @Orlando80

`0.3.1` - Better type checking by @Drinks, UMD wrapper by @SimenB. Now certifies against nodejs 12 and iojs (Thanks @SimenB).

`0.2.0` - [Fixes Bug #17](https://github.com/flitbit/diff/issues/17), [Fixes Bug #19](https://github.com/flitbit/diff/issues/19), [Enhancement #21](https://github.com/flitbit/diff/issues/21) Applying changes that are properly structured can now be applied as a change (no longer requires typeof Diff) - supports differences being applied after round-trip serialization to JSON format. Prefilter now reports the path of all changes - it was not showing a path for arrays and anything in the structure below (reported by @ravishvt).

*Breaking Change* &ndash; The structure of change records for differences below an array element has changed. Array indexes are now reported as numeric elements in the `path` if the changes is merely edited (an `E` kind). Changes of kind `A` (array) are only reported for changes in the terminal array itself and will have a nested `N` (new) item or a nested `D` (deleted) item.

`0.1.7` - [Enhancement #11](https://github.com/flitbit/diff/issues/11) Added the ability to filter properties that should not be analyzed while calculating differences. Makes `deep-diff` more usable with frameworks that attach housekeeping properties to existing objects. AngularJS does this, and the new filter ability should ease working with it.

`0.1.6` - Changed objects within nested arrays can now be applied. They were previously recording the changes appropriately but `applyDiff` would error. Comparison of `NaN` works more sanely - comparison to number shows difference, comparison to another `Nan` does not.

## Installation
```
npm install deep-diff
```

For the browser, you can install with [bower](http://bower.io/):

```
bower install deep-diff
```

## Tests

Tests use [mocha](http://mochajs.org/) and [expect.js](https://github.com/LearnBoost/expect.js/), so if you clone the [github repository](https://github.com/flitbit/json-ptr) you'll need to run:

```bash
npm install
```

... followed by ...

```bash
npm test
```

... or ...

```bash
mocha -R spec
```

### Importing

**nodejs**
```javascript
var deep = require('deep-diff')
```

**browser**
```html
<script src="deep-diff-0.3.1.min.js"></script>
```
> Minified, browser release of the current version of the module is under the `releases` folder.
> In a browser, `deep-diff` defines a global variable `DeepDiff`. If there is a conflict in the global namespace you can restore the conflicting definition and assign `deep-diff` to another variable like this: `var deep = DeepDiff.noConflict();`.

## Simple Examples

In order to describe differences, change revolves around an `origin` object. For consistency, the `origin` object is always the operand on the `left-hand-side` of operations. The `comparand`, which may contain changes, is always on the `right-hand-side` of operations.

``` javascript
var diff = require('deep-diff').diff;

var lhs = {
	name: 'my object',
	description: 'it\'s an object!',
	details: {
		it: 'has',
		an: 'array',
		with: ['a', 'few', 'elements']
	}
};

var rhs = {
	name: 'updated object',
	description: 'it\'s an object!',
	details: {
		it: 'has',
		an: 'array',
		with: ['a', 'few', 'more', 'elements', { than: 'before' }]
	}
};

var differences = diff(lhs, rhs);
```
*up to v 0.1.7* The code snippet above would result in the following structure describing the differences:
``` javascript
// Versions < 0.2.0
[ { kind: 'E',
    path: [ 'name' ],
    lhs: 'my object',
    rhs: 'updated object' },
  { kind: 'A',
    path: [ 'details', 'with' ],
    index: 2,
    item: { kind: 'E', path: [], lhs: 'elements', rhs: 'more' } },
  { kind: 'A',
    path: [ 'details', 'with' ],
    index: 3,
    item: { kind: 'N', rhs: 'elements' } },
  { kind: 'A',
    path: [ 'details', 'with' ],
    index: 4,
    item: { kind: 'N', rhs: { than: 'before' } } } ]
```

*v 0.2.0 and above* The code snippet above would result in the following structure describing the differences:
``` javascript
[ { kind: 'E',
    path: [ 'name' ],
    lhs: 'my object',
    rhs: 'updated object' },
  { kind: 'E',
    path: [ 'details', 'with', 2 ],
 		lhs: 'elements',
 		rhs: 'more' },
  { kind: 'A',
    path: [ 'details', 'with' ],
    index: 3,
    item: { kind: 'N', rhs: 'elements' } },
  { kind: 'A',
    path: [ 'details', 'with' ],
    index: 4,
    item: { kind: 'N', rhs: { than: 'before' } } } ]
```

### Differences

Differences are reported as one or more change records. Change records have the following structure:

* `kind` - indicates the kind of change; will be one of the following:
    * `N` - indicates a newly added property/element
    * `D` - indicates a property/element was deleted
    * `E` - indicates a property/element was edited
    * `A` - indicates a change occurred within an array
* `path` - the property path (from the left-hand-side root)
* `lhs` - the value on the left-hand-side of the comparison (undefined if kind === 'N')
* `rhs` - the value on the right-hand-side of the comparison (undefined if kind === 'D')
* `index` - when kind === 'A', indicates the array index where the change occurred
* `item` - when kind === 'A', contains a nested change record indicating the change that occurred at the array index

Change records are generated for all structural differences between `origin` and `comparand`. The methods only consider an object's own properties and array elements; those inherited from an object's prototype chain are not considered.

Changes to arrays are recorded simplistically. We care most about the shape of the structure; therefore we don't take the time to determine if an object moved from one slot in the array to another. Instead, we only record the structural
differences. If the structural differences are applied from the `comparand` to the `origin` then the two objects will compare as "deep equal" using most `isEqual` implementations such as found in [lodash](https://github.com/bestiejs/lodash) or [underscore](http://underscorejs.org/).

### Changes

When two objects differ, you can observe the differences as they are calculated and selectively apply those changes to the origin object (left-hand-side).
``` javascript
var observableDiff = require('deep-diff').observableDiff,
applyChange        = require('deep-diff').applyChange;

var lhs = {
	name: 'my object',
	description: 'it\'s an object!',
	details: {
		it: 'has',
		an: 'array',
		with: ['a', 'few', 'elements']
	}
};

var rhs = {
	name: 'updated object',
	description: 'it\'s an object!',
	details: {
		it: 'has',
		an: 'array',
		with: ['a', 'few', 'more', 'elements', { than: 'before' }]
	}
};

observableDiff(lhs, rhs, function (d) {
	// Apply all changes except those to the 'name' property...
	if (d.path.length !== 1 || d.path.join('.') !== 'name') {
		applyChange(lhs, rhs, d);
	}
});
```

## API Documentation

A standard import of `var diff = require('deep-diff')` is assumed in all of the code examples. The import results in an object having the following public properties:

* `diff(lhs, rhs, prefilter, acc)` &mdash; calculates the differences between two objects, optionally prefiltering elements for comparison, and optionally using the specified accumulator.
* `observableDiff(lhs, rhs, observer, prefilter)` &mdash; calculates the differences between two objects and reports each to an observer function, optionally, prefiltering elements for comparison.
* `applyDiff(target, source, filter)` &mdash; applies any structural differences from a source object to a target object, optionally filtering each difference.
* `applyChange(target, source, change)` &mdash; applies a single change record to a target object. NOTE: `source` is unused and may be removed.
* `revertChange(target, source, change)` reverts a single change record to a target object. NOTE: `source` is unused and may be removed.

### `diff`

The `diff` function calculates the difference between two objects.

**Arguments**

+ `lhs` - the left-hand operand; the origin object.
+ `rhs` - the right-hand operand; the object being compared structurally with the origin object.
+ `prefilter` - an optional function that determines whether difference analysis should continue down the object graph.
+ `acc` - an optional accumulator/array (requirement is that it have a `push` function). Each difference is pushed to the specified accumulator.

#### Pre-filtering Object Properties

The `prefilter`'s signature should be `function(path, key)` and it should return a truthy value for any `path`-`key` combination that should be filtered. If filtered, the difference analysis does no further analysis of on the identified object-property path.

# Compatibility

Currently testing on Travis CI against:

+ nodejs `6`
+ nodejs `5`
+ nodejs `4`
+ nodejs `0.12`
+ nodejs `0.11`
+ nodejs `0.10`

# Contributing

When contributing, keep in mind that it is an objective of `deep-diff` to have no package dependencies. This may change in the future, but for now, no-dependencies.

As of release 0.3.5, all edits/changes should be made to `index.es.js`. You must run the unit tests before submitting your PR: `npm test`. Hopefully your PR includes additional unit tests to illustrate your change/modification!

When you run `npm test`, linting will be performed and `index.js` will be built from `index.es.js`. Any linting errors will fail the tests... this includes code formatting.

This module still uses `jshint` but the plan is to switch to `eslint` very soon as I have done in several of my other modules.

**Thanks to all those who have contributed so far!**
