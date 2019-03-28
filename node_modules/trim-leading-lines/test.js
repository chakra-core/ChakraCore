'use strict';

require('mocha');
var assert = require('assert');
var trimLeadingLines = require('./');

describe('trim-leading-lines', function() {
  it('should export a function', function() {
    assert.equal(typeof trimLeadingLines, 'function');
  });

  it('should throw an error when invalid args are passed', function(cb) {
    try {
      trimLeadingLines();
      cb(new Error('expected an error'));
    } catch (err) {
      assert(err);
      assert.equal(err.message, 'expected a string');
      cb();
    }
  });

  it('should return empty strings', function() {
    assert.equal(trimLeadingLines(''), '');
  });

  it('should return original string if no newlines', function() {
    assert.equal(trimLeadingLines('foo'), 'foo');
  });

  it('should trim leading whitespace lines', function() {
    var fixture = [
      '                  ',
      '                  ',
      '                  ',
      '                  ',
      '                    GNU AFFERO GENERAL PUBLIC LICENSE',
      '                       Version 3, 19 November 2007',
      '',
    ].join('\n');

    var expected = [
      '                    GNU AFFERO GENERAL PUBLIC LICENSE',
      '                       Version 3, 19 November 2007',
      '',
    ].join('\n');

    assert.equal(trimLeadingLines(fixture), expected);
  });

  it('should trim leading empty lines', function() {
    var fixture = [
      '',
      '',
      '',
      '',
      '                    GNU AFFERO GENERAL PUBLIC LICENSE',
      '                       Version 3, 19 November 2007',
      '',
    ].join('\n');

    var expected = [
      '                    GNU AFFERO GENERAL PUBLIC LICENSE',
      '                       Version 3, 19 November 2007',
      '',
    ].join('\n');

    assert.equal(trimLeadingLines(fixture), expected);
  });

  it('should trim a mixture of leading empty and whitepace lines', function() {
    var fixture = [
      '                  ',
      '',
      '                  ',
      '',
      '                    GNU AFFERO GENERAL PUBLIC LICENSE',
      '                       Version 3, 19 November 2007',
      '',
    ].join('\n');

    var expected = [
      '                    GNU AFFERO GENERAL PUBLIC LICENSE',
      '                       Version 3, 19 November 2007',
      '',
    ].join('\n');

    assert.equal(trimLeadingLines(fixture), expected);
  });

  it('should not trim intermediate whitespace lines', function() {
    var fixture = [
      '                    GNU AFFERO GENERAL PUBLIC LICENSE',
      '                  ',
      '                  ',
      '                  ',
      '                  ',
      '                       Version 3, 19 November 2007',
      '',
    ].join('\n');

    assert.equal(trimLeadingLines(fixture), fixture);
  });
});
