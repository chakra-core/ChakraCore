# babel-plugin-ember-modules-api-polyfill

[![Greenkeeper badge](https://badges.greenkeeper.io/ember-cli/babel-plugin-ember-modules-api-polyfill.svg)](https://greenkeeper.io/)

> This plugin transforms [JavaScript modules API](https://github.com/emberjs/rfcs/blob/master/text/0176-javascript-module-api.md) import statements
> back to the legacy "global" ember object syntax

## Example

```js
import { inject } from "@ember/service"
```
back to the legacy
```js
const inject = Ember.inject.service
```

## Installation

`npm install --save babel-plugin-ember-modules-api-polyfill`

## Why

This plugin provides an API polyfill to allow ember addon authors to adopt the new
[JavaScript modules API](https://github.com/emberjs/rfcs/blob/master/text/0176-javascript-module-api.md) whilst still maintaining backwards 
compatibility with older versions of Ember that do not support the new modules API.

The intention of this Babel plugin is to also allow for a transition period and allow applications to exist in a mixed state whilst transitioning 
from the old "global" ember object pattern, into the new modular pattern.

# How

Using the [ember-rfc176-data](https://github.com/ember-cli/ember-rfc176-data) package, that contains the official mapping of old global
object names to the new JS modules API import statements, addons that adopt the new API can be transpiled back to the legacy format if Ember-CLI
detects that the host application ember version does not support the new modules API.

The plugin supports both default `import Component from "@ember/component"` and named `import { inject } from "@ember/service"` import statements,
converting their syntax back to separate `const` variables within the source file. This transpilation is done at compile time by Ember CLI.

In order for ember addon developers to adopt this new API syntax, they must declare a dependency on `ember-cli-babel:v6.6.0` or above in their
package.json:

```json
{
  "dependencies": {
    "ember-cli-babel": "^6.6.0"
  }
}
```
