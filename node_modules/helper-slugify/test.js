'use strict';

require('mocha');
var assert = require('assert');
var Engine = require('engine');
var template = require('lodash.template');
var slugify = require('./');
var imports = {imports: {slugify: slugify}};
var engine;

describe('helper-slugify', function() {
  beforeEach(function() {
    engine = new Engine();
    engine.helper('slugify', slugify);
  });

  it('should export a function', function() {
    assert.equal(typeof slugify, 'function');
  });

  it('should return an empty string when undefined.', function() {
    assert.equal(template('<%= slugify() %>', imports)(), '');
    assert.equal(engine.render('<%= slugify() %>'), '');
  });

  it('should slugify the characters in a string.', function() {
    assert.equal(engine.render('<%= slugify("foo bar baz") %>'), 'foo-bar-baz');
  });

  it('should slugify the characters in a string.', function() {
    assert.equal(template('<%= slugify("foo bar baz") %>', imports)(), 'foo-bar-baz');
    assert.equal(engine.render('<%= slugify("foo bar baz") %>'), 'foo-bar-baz');
  });

  it('should work with hyphens.', function() {
    assert.equal(template('<%= slugify("foo-bar-baz") %>', imports)(), 'foo-bar-baz');
    assert.equal(engine.render('<%= slugify("foo-bar-baz") %>'), 'foo-bar-baz');
    assert.equal(template('<%= slugify("-foo bar baz-") %>', imports)(), '-foo-bar-baz-');
    assert.equal(engine.render('<%= slugify("-foo bar baz-") %>'), '-foo-bar-baz-');
  });

  it('should work with other non-word characters.', function() {
    assert.equal(template('<%= slugify("9foo-bar_baz") %>', imports)(), '9foo-bar_baz');
    assert.equal(engine.render('<%= slugify("9foo-bar_baz") %>'), '9foo-bar_baz');
    assert.equal(template('<%= slugify("_foo_bar_baz-") %>', imports)(), '_foo_bar_baz-');
    assert.equal(engine.render('<%= slugify("_foo_bar_baz-") %>'), '_foo_bar_baz-');
  });
});
