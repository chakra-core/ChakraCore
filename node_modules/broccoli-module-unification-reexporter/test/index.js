'use strict';

const helper = require('broccoli-test-helper');
const co = require('co');
const expect = require('chai').expect;
const createBuilder = helper.createBuilder;
const createTempDir = helper.createTempDir;
const ModuleUnificationReexporter = require('../index');

describe('broccoli-module-unification-reexporter', function() {
  let input, output;

  beforeEach(co.wrap(function*() {
    input = yield createTempDir();

    const subject = new ModuleUnificationReexporter(input.path(), {
      namespace: 'this-addon-name',
    });

    output = createBuilder(subject);
  }));

  afterEach(co.wrap(function*() {
    yield input.dispose();

    if (output) {
      yield output.dispose();
    }
  }));

  function testExpectation(description, inputTree, outputTree) {
    it(description, co.wrap(function*() {
      // INITIAL
      input.write(inputTree);

      yield output.build();

      expect(output.read()).to.deep.equal(outputTree);

      yield output.build();

      expect(output.changes()).to.deep.equal({});
    }));
  }

  testExpectation(
    'should handle mixed template + JS components', {
      ui: {
        components: {
          'main': {
            'template.hbs': '{{#if true}} yay {{/if}}',
            'component.js': 'export default Ember.Component'
          },
          'foo-bar-baz': {
            'template.hbs': '{{#if true}} yay {{/if}}',
            'component.js': 'export default Ember.Component'
          },
          'other-derp': {
            'component.js': 'export default Ember.Component'
          },
          'lolerskates-here': {
            'template.hbs': '{{#if true}} lol {{/if}}'
          }
        }
      }
    }, {
      components: {
        'this-addon-name.js': `export { default } from 'this-addon-name/src/ui/components/main/component';`,
        'foo-bar-baz.js': `export { default } from 'this-addon-name/src/ui/components/foo-bar-baz/component';`,
        'other-derp.js': `export { default } from 'this-addon-name/src/ui/components/other-derp/component';`
      },
      templates: {
        components: {
          'this-addon-name.js': `export { default } from 'this-addon-name/src/ui/components/main/template';`,
          'foo-bar-baz.js': `export { default } from 'this-addon-name/src/ui/components/foo-bar-baz/template';`,
          'lolerskates-here.js': `export { default } from 'this-addon-name/src/ui/components/lolerskates-here/template';`
        }
      }
    }
  );


  it('should handle helper reexports');
  it('should handle initializers');
  it('should handle instance-initializers');
  it('should handle partials?');
  it('should allow custom mappings as overrides');
  it('should allow static mappings (no auto-detection)');
  it('should run only on the first build');
  it('should allow `forceOnRebuilds`');
});
