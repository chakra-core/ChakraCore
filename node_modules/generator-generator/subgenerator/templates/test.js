'use strict';
const path = require('path');
const assert = require('yeoman-assert');
const helpers = require('yeoman-test');

describe('<%- generatorName %>:<%- name %>', () => {
  beforeAll(() => {
    return helpers
      .run(path.join(__dirname, '../generators/<%- name %>'))
      .withPrompts({ someAnswer: true });
  });

  it('creates files', () => {
    assert.file(['dummyfile.txt']);
  });
});
