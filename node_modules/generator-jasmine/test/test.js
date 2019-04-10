/*global describe, beforeEach, it */
'use strict';
const path = require('path');
const helpers = require('yeoman-test');
const assert = require('yeoman-assert');

describe('Jasmine generator test', () => {
  before(done => {
    helpers.run(path.join(__dirname, '../generators/app'))
      .inDir(path.join(__dirname, 'temp'))
      .withOptions({'skip-install': true})
      .on('end', done);
  });

  it('creates expected files', () => {
    assert.file([
      'test/spec/test.js',
      'test/index.html'
    ]);
  });
});
