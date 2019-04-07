'use strict';

var assert = require('assert');
var sortAsc = require('./');

describe('sort ascending', function () {
  it('should sort elements in ascending order.', function () {
    assert.deepEqual(['d', 'c', 'b', 'a'].sort(sortAsc), ['a', 'b', 'c', 'd']);
    assert.deepEqual(['g', 'z', 'a', 'x'].sort(sortAsc), ['a', 'g', 'x', 'z']);
    assert.deepEqual(['z', 'z', 'a', 'z'].sort(sortAsc), ['a', 'z', 'z', 'z']);
    assert.deepEqual(['zz', 'z', 'aa', 'a'].sort(sortAsc), ['a', 'aa', 'z', 'zz']);
    assert.deepEqual(['aba', 'aab', 'acc', 'abb', 'aabb'].sort(sortAsc), ['aab', 'aabb', 'aba', 'abb', 'acc']);
  });
});
