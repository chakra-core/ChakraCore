'use strict';
/* globals QUnit */

const describe = QUnit.module;
const it = QUnit.test;
const babel = require('babel-core');
const Plugin = require('../src');
const mapping = require('ember-rfc176-data');

function transform(source, _plugins) {
  let plugins = _plugins || [
    [Plugin],
  ];
  let result = babel.transform(source, {
    plugins,
  });

  return result.code;
}

function matches(source, expected, only) {
  (only ? QUnit.only : it)(`${source}`, assert => {
    let actual = transform(source);

    assert.equal(actual, expected);
  });
}

// Ensure each of the config mappings is mapped correctly
mapping.forEach(exportDefinition => {
  const importRoot = exportDefinition.module;

  let importName = exportDefinition.export;
  if (!importName) {
    importName = 'default';
  }
  const varName = importName === 'default' ? 'defaultModule' : importName;
  const localName = varName === 'defaultModule' ? varName : `{ ${varName} }`;

  describe(`ember-modules-api-polyfill-${importRoot}-with-${importName}`, () => {
    matches(
      `import ${localName} from '${importRoot}';var _x = ${varName}`,
      `var _x = ${exportDefinition.global};`
    );
  });
});

// Ensure it works in complex scopes
describe(`ember-modules-api-polyfill-import-complex-scopes`, () => {
  matches(
    `import { isEmpty } from '@ember/utils';
var _x = someArray.every(item => isEmpty(item));
var _y = someOtherArray.some((isEmpty, idx) => isEmpty(idx));`,
    `
var _x = someArray.every(item => Ember.isEmpty(item));
var _y = someOtherArray.some((isEmpty, idx) => isEmpty(idx));`
  );
});

// Ensure mapping without reference just removes the line
describe(`ember-modules-api-polyfill-import-without-reference`, () => {
  matches(
    `import { empty } from '@ember/object/computed';`,
    ``
  );
});

// Ensure mapping multiple imports makes multiple variables
describe(`ember-modules-api-polyfill-import-multiple`, () => {
  matches(
    `import { empty, notEmpty } from '@ember/object/computed';var _x = empty;var _y = notEmpty;`,
    `var _x = Ember.computed.empty;var _y = Ember.computed.notEmpty;`
  );
});

// Ensure jQuery and RSVP imports work
describe(`ember-modules-api-polyfill-named-as-alias`, () => {
  matches(
    `import jQuery from 'jquery'; import RSVP from 'rsvp';var $ = jQuery;var _y = RSVP`,
    `var $ = Ember.$;var _y = Ember.RSVP;`
  );
});

// Ensure mapping a named aliased import
describe(`ember-modules-api-polyfill-named-as-alias`, () => {
  matches(
    `import { empty as isEmpty } from '@ember/object/computed';var _x = isEmpty;`,
    `var _x = Ember.computed.empty;`
  );
});

// Ensure mapping a named and aliased import makes multiple named variables
describe(`ember-modules-api-polyfill-import-named-multiple`, () => {
  matches(
    `import { empty, notEmpty as foo } from '@ember/object/computed';var _x = empty;var _y = foo;`,
    `var _x = Ember.computed.empty;var _y = Ember.computed.notEmpty;`
  );
});

// Ensure mapping the default as an alias works
describe(`ember-modules-api-polyfill-default-as-alias`, () => {
  matches(
    `import { default as foo } from '@ember/component';var _x = foo;`,
    `var _x = Ember.Component;`
  );
});

// Ensure reexporting things works
describe(`ember-modules-api-polyfill-reexport`, () => {
  matches(
    `export { default } from '@ember/component';`,
    `export default Ember.Component;`
  );

  matches(
    `export { default as Component } from '@ember/component';`,
    `export var Component = Ember.Component;`
  );

  matches(
    `export { computed } from '@ember/object';`,
    `export var computed = Ember.computed;`
  );

  matches(
    `export { computed as foo } from '@ember/object';`,
    `export var foo = Ember.computed;`
  );

  matches(
    `export var foo = 42;`,
    `export var foo = 42;`
  );

  it(`throws an error for wildcard imports`, assert => {
    let input = `import * as debug from '@ember/debug';`;

    assert.throws(() => {
      transform(input, [
        [Plugin],
      ]);
    }, 'Using `import * as debug from \'@ember/debug\'` is not supported');
  });

  it(`throws an error for wildcard exports`, assert => {
    let input = `export * from '@ember/object/computed';`;

    assert.throws(() => {
      transform(input, [
        [Plugin],
      ]);
    }, /Wildcard exports from @ember\/object\/computed are currently not possible/);
  });

  matches(
    `export * from 'foo';`,
    `export * from 'foo';`
  );
});

// Ensure unknown exports are not removed
describe(`unknown imports from known module`, () => {
  it(`allows blacklisting import paths`, assert => {
    let input = `import { derp } from '@ember/object/computed';`;

    assert.throws(() => {
      transform(input, [
        [Plugin],
      ]);
    }, /@ember\/object\/computed does not have a derp export/);
  });
});

describe(`import then export`, () => {
  matches(
    `import { capitalize } from '@ember/string';
export { capitalize };`,
    `var capitalize = Ember.String.capitalize;

export { capitalize };`
  );
  matches(
    `import { capitalize, camelize } from '@ember/string';
    camelize("a thing");
    capitalize("another thing");
    export { capitalize };`,
    `var capitalize = Ember.String.capitalize;

Ember.String.camelize("a thing");
capitalize("another thing");
export { capitalize };`
  );
});

describe('options', () => {
  describe('blacklist', () => {
    it(`allows blacklisting import paths`, assert => {
      let input = `import { assert } from '@ember/debug';`;
      let actual = transform(input, [
        [Plugin, { blacklist: ['@ember/debug'] }],
      ]);

      assert.equal(actual, input);
    });

    it(`allows blacklisting specific named imports`, assert => {
      let input = `import { assert, inspect } from '@ember/debug';var _x = inspect`;
      let actual = transform(input, [
        [Plugin, { blacklist: { '@ember/debug': ['assert', 'warn', 'deprecate'] } }],
      ]);

      assert.equal(actual, `import { assert } from '@ember/debug';var _x = Ember.inspect;`);
    });

    it('does not error when a blacklist is not present', assert => {
      let input = `import { assert, inspect } from '@ember/debug';var _x = assert; var _y = inspect;`;
      let actual = transform(input, [
        [Plugin, { blacklist: { } }],
      ]);

      assert.equal(actual, `var _x = Ember.assert;var _y = Ember.inspect;`);
    });
  });
});

describe(`import from 'ember'`, () => {
  matches(
    `import Ember from 'ember';var _x = Ember;`,
    `var _x = Ember;`
  );
  matches(
    `import Em from 'ember'; var _x = Em;`,
    `var _x = Ember;`
  );
  matches(
    `import Asdf from 'ember';var _x = Asdf;`,
    `var _x = Ember;`
  );
  matches(
    `import './foo';`,
    `import './foo';`
  );
});

describe(`import without specifier is removed`, () => {
  matches(
    `import 'ember';`,
    ``
  );
  matches(
    `import '@ember/component';`,
    ``
  );
});

