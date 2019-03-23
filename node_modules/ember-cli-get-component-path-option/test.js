var getPathOption = require('./');
var expect = require('chai').expect;

describe('getPathOption', function() {
  it('throws without correct input', function() {
    expect(getPathOption).to.throw(/getPathOptions first argument must be an object/);
    expect(function() {
      getPathOption(null);
    }).to.throw(/getPathOptions first argument must be an object/);
    expect(function() {
      getPathOption(undefined);
    }).to.throw(/getPathOptions first argument must be an object/);
    expect(function() {
      getPathOption(1);
    }).to.throw(/getPathOptions first argument must be an object/);
  });

  it('no path, should just result componets ', function() {
    expect(getPathOption({})).to.eql('components');
  });

  it('empty path, should be empty', function() {
    expect(getPathOption({
      path: ''
    })).to.eql('');
  });

  it('path, should be path', function() {
    expect(getPathOption({
      path: 'foo/bar'
    })).to.eql('foo/bar');
  });
});
