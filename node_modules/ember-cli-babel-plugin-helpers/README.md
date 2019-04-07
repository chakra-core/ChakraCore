# ember-cli-babel-plugin-helpers [![Build Status](https://travis-ci.com/dfreeman/ember-cli-babel-plugin-helpers.svg?branch=master)](https://travis-ci.com/dfreeman/ember-cli-babel-plugin-helpers)

This package provides utilities for managing Babel plugins in configuration for Ember CLI apps and addons. The primary expected consumers are addons that want to set up plugins on behalf of their parent, while ensuring that the same plugin isn't registered multiple times.

## Example

As a simple example, the addon implementaiton below would check whether its parent has the [plugin](https://babeljs.io/docs/en/next/babel-plugin-proposal-class-properties.html) for the [class properties proposal](https://github.com/tc39/proposal-class-fields) set up, adding it if not.

```js
const { hasPlugin, addPlugin } = require('ember-cli-babel-plugin-helpers');

module.exports = {
  name: require('./package').name,

  included(parent) {
    this._super.included.apply(this, arguments);

    if (!hasPlugin(parent, '@babel/plugin-proposal-class-properties')) {
      addPlugin(parent, require.resolve('@babel/plugin-proposal-class-properties'));
    }
  }
};
```

## Usage

### `hasPlugin(target, pluginName)`

Indicates whether a plugin with the given name is already present in the given target's Babel config.

The `target` may be an array of Babel plugin configurations, or an `EmberApp` or `Addon` instance, and `pluginName` may be the full or shorthand name for a Babel plugin (following Babel's [normalization rules](https://babeljs.io/docs/en/next/options#name-normalization)).

Note that `hasPlugin` will make a best-effort attempt at determining a normalized plugin name to compare to for each existing configured plugin. If, for example, the path `/path/to/node_modules/@babel/plugin-proposal-class-features/lib/entry.js` (as returned by e.g. `require.resolve`) were one of the configured plugins, the package name would be extracted from that path and `hasPlugin(target, '@babel/proposal-class-features')` would return `true`.

### `findPlugin(target, pluginName)`

Operates like `hasPlugin(target, pluginName)`, but returns the corresponding entry in the configured `plugins` array, if any. This may be useful if you care about more than just the presence of a given plugin, and want to check e.g. its configuration for the presence of a particular option.

### `addPlugin(target, pluginConfig, options)`

Adds a plugin to the given target's Babel config.

The `target` may be an array of Babel plugin configurations, or an `EmberApp` or `Addon` instance, and any necessary configuration containers (e.g. the `babel` hash or the `plugins` array) will be created on the target if they don't already exist.

The `pluginConfig` may be either an array containing up to three elements or a plain string. The [Babel documentation](https://babeljs.io/docs/en/next/options#plugin-preset-entries) has further details on plugin configuration entries, but at a high level the array elements are:
 - A string identifying the plugin to add, either the name of the plugin (e.g. `@babel/plugin-proposal-class-properties`) or the full path to its entry point. Using `require.resolve` to produce the full path is recommended for addons that wish to add specific plugins to ensure that the plugin is loaded correctly by the parent's Babel instance.
 - An optional configuration object specifying options for the plugin.
 - An optional unique identifier.

An optional `options` hash may also be passed, specifying constraints for where the plugin should be placed relative to others if they're present.

 - `before`: an array of plugin names (following the same rules as `hasPlugin`) the given plugin must appear _earlier than_ if they're present in the existing configuration
 - `after`: an array of plugin names that the given plugin must appear _later than_ if they're present in the existing configuration

If the given `before` and `after` constraints result in a conflict because of the order of existing plugins, `addPlugin` will raise an exception.

If no `before` or `after` constraints are set, the plugin will be added at the end of the array of existing plugin configurations.

## Notes on Stringency

While these utilities make a best effort to identify existing plugins that are already configured, there are cases where it may be impossible to identify whether a plugin is present or not.

For instance, Babel will accept an actual plugin implementation in the `plugins` array rather than a string identifying the module to load, and if a plugin has been specified this way, it's typically not possible to determine where that plugin came from, and `hasPlugin` may report `false` in such cases.

These utilities also identify plugins by their package name, which is typically how Babel plugins are distributed. If, however, a single package exposes multiple plugins, `hasPlugin` and `findPlugin` won't distinguish between the two.
