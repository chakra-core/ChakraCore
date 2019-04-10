'use strict';

var bowerLatest = require('../lib/bower-latest.js');
var should = require('should');

describe('bowerLatest', function () {

  it('should return undefined when no package is found', function (cb) {

    this.timeout(15000);

    // I hope that asdf!@#$%^&*() will never be a name bower compontent
    bowerLatest('asdf!@#$%^&*()', function(compontent) {
      should.not.exist(compontent);
      cb();
    });
  });

  it('should return a package', function (cb) {

    this.timeout(15000);

    bowerLatest('backbone', function(compontent) {
      should(compontent).have.property('name');
      should(compontent).have.property('version');
      cb();
    });
  });

});
