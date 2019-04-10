var bytewise = require('../');
var typewise = require('typewise-core');
var util = require('typewise-core/test/util');
var tape = require('tape');

var expected = [
  -4,
  -0.304958230,
  0,
  0.304958230,
  4,
  'bar',
  'baz',
  'foo',
  [ 0, 0, 'foo' ],
  [ 0, 1, 'foo' ],
  [ 0, 1, 'foo', 0 ],
  [ 0, 1, 'foo', 1 ],
  [ 0, 'bar', 'baz' ],
  [ 0, 'foo' ],
  [ 0, 'foo', 'bar' ],
  [ 0, 'foo', [] ],
  [ 0, 'foo', [ 'bar' ] ],
  [ 0, 'foo', [ 'bar' ], [] ],
  [ 0, 'foo', [ 'bar' ], [ 'foo' ] ],
  [ 0, 'foo', [ 'bar', 'baz' ] ],
  [ 1, 'bar', 'baz' ],
  [ 1, 'bar', 'baz' ],
  [ 'foo', 'bar', 'baz' ],
  [ 'foo', [ 'bar', 'baz' ] ],
  [ 'foo', [ 'bar', [ 'baz' ] ] ],
];

var shuffled = util.shuffle(expected.slice());

tape('sorts in expected order', function (t) {
  t.equal(
    bytewise.encode(shuffled.sort(typewise.compare)).toString('hex'),
    bytewise.encode(expected).toString('hex')
  );
  t.end();
});

tape('sorts with same order when encoded', function (t) {
  var decoded = shuffled
    .map(bytewise.encode)
    .sort(bytewise.compare)
    .map(bytewise.decode);

  t.equal(
    bytewise.encode(decoded).toString('hex'),
    bytewise.encode(expected).toString('hex')
  );
  t.end();
});
