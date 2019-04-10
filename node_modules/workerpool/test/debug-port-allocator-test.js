'use strict';

var assert = require('assert');
var DebugPortAllocator = require('../lib/debug-port-allocator');

describe('DebugPortAllocator', function () {
  it('works', function() {
    var allocator = new DebugPortAllocator();
    assert.equal(allocator.length, 0);
    assert.equal(allocator.nextAvailableStartingAt(5), 5);
    assert.equal(allocator.length, 1);
    assert.equal(allocator.nextAvailableStartingAt(5), 6);
    assert.equal(allocator.length, 2);
    assert.equal(allocator.nextAvailableStartingAt(5), 7);
    assert.equal(allocator.length, 3);
    assert.equal(allocator.nextAvailableStartingAt(4), 4);
    assert.equal(allocator.length, 4);
    assert.equal(allocator.nextAvailableStartingAt(4), 8);
    assert.equal(allocator.length, 5);

    allocator.releasePort(8);
    assert.equal(allocator.length, 4);
    assert.equal(allocator.nextAvailableStartingAt(8), 8);
    allocator.releasePort(8);
    allocator.releasePort(7);
    allocator.releasePort(6);
    allocator.releasePort(5);
    allocator.releasePort(4);
    assert.equal(allocator.length, 0);

    assert.throws(function() {
      allocator.nextAvailableStartingAt(65535);
    }, /WorkerPool debug port limit reached/);
    assert.throws(function() {
      allocator.nextAvailableStartingAt(75535);
    }, /WorkerPool debug port limit reached/);
  })
});

