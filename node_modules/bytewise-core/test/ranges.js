var bytewise = require('../')
var tape = require('tape')
var util = require('typewise-core/test/util')

var ARRAY = bytewise.sorts.array
var STRING = bytewise.sorts.string

function eqHex(t, range, hexExpected) {
  t.equal(bytewise.encode(range).toString('hex'), hexExpected)
}

function decodeThrows(t, range) {
  var encoded = bytewise.encode(range)
  // t.ok(encoded.undecodable, 'range should have undecodable bit set')
  t.throws(function () {
    bytewise.decode(encoded)
  }, 'cannot decode a range')
}

tape('bounded ranges', function (t) {
  var range = bytewise.bound.lower()
  eqHex(t, range, '00')
  decodeThrows(t, range)

  range = bytewise.bound.upper()
  eqHex(t, range, 'ff')
  decodeThrows(t, range)
  t.end()
})

tape('bounded arrays', function (t) {
  eqHex(t, [ 'foo' ], 'a070666f6f0000')

  var range = [ 'foo', bytewise.bound.lower() ]
  eqHex(t, range, 'a070666f6f000000')
  decodeThrows(t, range)

  range = [ 'foo', bytewise.bound.upper() ]
  eqHex(t, range, 'a070666f6f00ff00')
  decodeThrows(t, range)
  t.end()
})

tape('prefix-bounded arrays', function (t) {
  var prefix = bytewise.encode([ 'foo', 'bar' ])

  var range = ARRAY.bound.lower([ 'foo', 'bar' ])
  t.equal(
    bytewise.encode(range).toString('hex'),
    prefix.toString('hex').slice(0, -2)
  )
  decodeThrows(t, range)

  range = ARRAY.bound.upper([ 'foo', 'bar' ])
  t.equal(
    bytewise.encode(range).toString('hex'),
    prefix.toString('hex').slice(0, -2) + 'ff'
  )
  decodeThrows(t, range)
  t.end()
})

tape('bounded ranges, nested', function (t) {
  eqHex(t, [ 'foo', [ 'bar' ] ], 'a070666f6f00a070626172000000')

  var range = [ 'foo', [ 'bar', bytewise.bound.lower() ] ]
  eqHex(t, range, 'a070666f6f00a07062617200000000')
  decodeThrows(t, range)

  eqHex(t, [ 'foo', [ 'bar' ] ], 'a070666f6f00a070626172000000')
  range = [ 'foo', [ 'bar', bytewise.bound.upper() ] ]
  eqHex(t, range, 'a070666f6f00a07062617200ff0000')
  decodeThrows(t, range)
  t.end()
})

tape('prefix-bounded strings', function (t) {
  eqHex(t, 'baz', '7062617a')

  var range = STRING.bound.lower('baz')
  eqHex(t, range, '7062617a')
  decodeThrows(t, range)

  range = STRING.bound.upper('baz')
  eqHex(t, range, '7062617aff')
  decodeThrows(t, range)
  t.end()
})

tape('prefix-bounded strings, nested', function (t) {
  eqHex(t, [ 'foo', [ 'bar', 'baz' ] ], 'a070666f6f00a070626172007062617a000000')

  var range = [ 'foo', [ 'bar', STRING.bound.lower('baz') ] ]
  eqHex(t, range, 'a070666f6f00a070626172007062617a0000')
  decodeThrows(t, range)

  range = [ 'foo', [ 'bar', STRING.bound.upper('baz') ] ]
  eqHex(t, range, 'a070666f6f00a070626172007062617aff0000')
  decodeThrows(t, range)
  t.end()
})
