'use strict';

import shouldPreserveSymlinks = require('../lib/should-preserve-symlinks');
import chai = require('chai');
const expect = chai.expect;

describe('shouldPreserveSymlinks', function() {
  it('works', function() {
    expect(shouldPreserveSymlinks({ execArgv: [], env: {  NODE_PRESERVE_SYMLINKS: '1'}})).to.eql(true);
    expect(shouldPreserveSymlinks({ execArgv: [], env: {  NODE_PRESERVE_SYMLINKS: 'false'}})).to.eql(true);
    expect(shouldPreserveSymlinks({ execArgv: [], env: {  NODE_PRESERVE_SYMLINKS: ''}})).to.eql(false);
    expect(shouldPreserveSymlinks({ execArgv: [], env: {  foo: 'bar' }})).to.eql(false);

    expect(shouldPreserveSymlinks({ execArgv: ['--preserve-symlinks'], env: {}})).to.eql(true);
    expect(shouldPreserveSymlinks({ execArgv: ['--a', '--preserve-symlinks', '--b'], env: {}})).to.eql(true);
    expect(shouldPreserveSymlinks({ execArgv: [], env: {}})).to.eql(false);
  });
});
