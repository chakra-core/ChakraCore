# Babel Debug Macros And Feature Flags

This provides debug macros and feature flagging.

## Setup

The plugin takes 4 types options: `flags`, `svelte`, `debugTools`, and
`externalizeHelpers`. The `importSpecifier` is used as a hint to this plugin as
to where macros are being imported and completely configurable by the host.

Like Babel you can supply your own helpers using the `externalizeHelpers`
options.

```js
{
  plugins: [
    ['babel-debug-macros', {
      // @required
      debugTools: {
        isDebug: true,
        source: 'debug-tools',
        // @optional
        assertPredicateIndex: 0
      },

      flags: [
        { source: '@ember/env-flags', flags: { DEBUG: true } },
        {
          name: 'ember-source',
          source: '@ember/features',
          flags: {
            FEATURE_A: false,
            FEATURE_B: true,
            DEPRECATED_CONTROLLERS: "2.12.0"
          }
        }
      ],

      // @optional
      svelte: {
        'ember-source': "2.15.0"
      },

      // @optional
      externalizeHelpers: {
        module: true,
        // global: '__my_global_ns__'
      }
    }]
  ]
}
```

Flags and features are inlined into the consuming module so that something like UglifyJS will DCE them when they are unreachable.

## Simple environment and feature flags

```javascript
import { DEBUG } from '@ember/env-flags';
import { FEATURE_A, FEATURE_B } from '@ember/features';

if (DEBUG) {
  console.log('Hello from debug');
}

let woot;
if (FEATURE_A) {
  woot = () => 'woot';
} else if (FEATURE_B) {
  woot = () => 'toow';
}

woot();
```

Transforms to:

```javascript
if (true /* DEBUG */) {
  console.log('Hello from debug');
}

let woot;
if (false /* FEATURE_A */) {
  woot = () => 'woot';
} else if (true) {
  woot = () => 'toow';
}

woot();
```

## `warn` macro expansion

```javascript
import { warn } from 'debug-tools';

warn('this is a warning');
```

Expands into:

```javascript
(true && console.warn('this is a warning'));
```

## `assert` macro expansion

The `assert` macro can expand in a more intelligent way with the correct
configuration. When `babel-plugin-debug-macros` is provided with the
`assertPredicateIndex` the predicate is injected in front of the assertion
in order to avoid costly assertion message generation when not needed.

```javascript
import { assert } from 'debug-tools';

assert((() => {
  return 1 === 1;
})(), 'You bad!');
```

With the `debugTools: { assertPredicateIndex: 0 }` configuration the following expansion is done:

```js
(true && !((() => { return 1 === 1;})()) && console.assert(false, 'this is a warning'));
```

When `assertPredicateIndex` is not specified, the following expansion is done:

```javascript
(true && console.assert((() => { return 1 === 1;})(), 'this is a warning'));
```

## `deprecate` macro expansion

```javascript
import { deprecate } from 'debug-tools';

let foo = 2;

deprecate('This is deprecated.', foo % 2);
```

Expands into:

```javascript
let foo = 2;

(true && !(foo % 2) && console.warn('This is deprecated.'));
```

## Externalized Helpers

When you externalize helpers you must provide runtime implementations for the
above macros. An expansion will still occur, however we will emit references to
those runtime helpers.

A global expansion looks like the following:

```javascript
import { warn } from 'debug-tools';

warn('this is a warning');
```

Expands into:

```javascript
(true && Ember.warn('this is a warning'));
```

While externalizing the helpers to a module looks like the following:

```javascript
import { warn } from 'debug-tools';

warn('this is a warning');
```

Expands into:

```javascript
(true && warn('this is a warning'));
```

# Svelte

Svelte allows for consumers to opt into stripping deprecated code from your
dependecies. By adding a package name and minimum version that contains no
deprecations, that code will be compiled away.

For example, consider you are on `ember-source@2.10.0` and you have no
deprecations. All deprecated code in `ember-source` that is `<=2.10.0` will be
removed.

```

svelte: {
  "ember-source": "2.10.0"
}

```

Now if you bump to `ember-source@2.11.0` you may encounter new deprecations.
The workflow would then be to clear out all deprecations and then bump the
version in the `svelte` options.

```
svelte: {
  "ember-source": "2.11.0"
}
```
