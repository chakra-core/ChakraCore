/*!
 * reduce-object <https://github.com/jonschlinkert/reduce-object>
 *
 * Copyright (c) 2014 Jon Schlinkert, contributors.
 * Licensed under the MIT License
 */

'use strict';

var should = require('should');
var reduce = require('./');

describe('.reduce()', function () {
  it('should return the accumulated result after running each property in the obj through the callback.', function () {
    var a = {a: 'foo', b: 'bar', c: {}};
    var obj = reduce(a, function (acc, value, key, orig) {
      if (typeof value === 'object') {
        acc[key] = {what: 'huh?'};
      } else {
        acc[key] = value.toUpperCase(); // why?
      }
      return acc;
    }, {});

    obj.should.eql({a: 'FOO', b: 'BAR', c: {what: 'huh?'}});
  });
});
