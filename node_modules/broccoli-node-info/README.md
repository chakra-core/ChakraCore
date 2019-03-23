# broccoli-node-info

[![Build Status](https://travis-ci.org/broccolijs/broccoli-node-info.svg?branch=master)](https://travis-ci.org/broccolijs/broccoli-node-info)

This is a low-level package used to communicate with Broccoli nodes (that is,
plugin instances). It provides a thin wrapper around
`node.__broccoliGetInfo__()`, normalizing different versions of the node API
to the newest version.

This package is mainly used by the Broccoli `Builder` class, but it can also
be used for inspecting nodes for testing, wrapping, diagnostics, or similar
purposes. If you are tempted to call `node.__broccoliGetInfo__()` or access
private variables on
[broccoli-plugin](https://github.com/broccolijs/broccoli-plugin) instances,
such as `node._name` or `node._inputNodes`, then use this package instead!

For background on the Broccoli node API, see
[docs/node-api.md](https://github.com/broccolijs/broccoli/blob/master/docs/node-api.md).

## Usage

```js
var broccoliNodeInfo = require('broccoli-node-info');

node = new SomeBroccoliPlugin();

var nodeInfo = broccoliNodeInfo.getNodeInfo(node);
// Now we can use nodeInfo.name, nodeInfo.annotation, etc.
```

### `getNodeInfo(node)`

This function calls `node.__broccoliGetInfo__(broccoliNodeInfo.features)`.

For `node` objects conforming to the most recent node API, it simply returns
the resulting `nodeInfo` object.

For `node` object conforming to an older version of the node API, it provides
compatibility code to normalize the `nodeInfo` object.

As a result, regardless of the API version used by the `node` object, we
obtain a `nodeInfo` object conforming to the current specification in
[docs/node-api.md](https://github.com/broccolijs/broccoli/blob/master/docs/node-api.md#the-nodeinfo-object)

This function throws a `broccoliNodeInfo.InvalidNodeError` when called with

* a plain string node (while still supported by the Broccoli `Builder`, they
  are deprecated in favor of
  [broccoli-source](https://github.com/broccolijs/broccoli-source)),
* a node using the old `.read/.rebuild` API (see
  [docs/broccoli-1-0-plugin-api.md](https://github.com/broccolijs/broccoli/blob/master/docs/broccoli-1-0-plugin-api.md)), or
* some other object that isn't a node.

### `InvalidNodeError`

`Error` subclass, potentially thrown by `getNodeInfo()`.

### `features`

A hash of all supported feature flags. This hash acts like a version number
for the node API. In the current version, its value is

```js
{
  persistentOutputFlag: true,
  sourceDirectories: true
}
```
