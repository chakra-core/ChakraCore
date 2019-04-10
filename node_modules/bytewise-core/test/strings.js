var bytewise = require('../');
var compare = require('../util').compare
var tape = require('tape');
var util = require('typewise-core/test/util');

function push(prefix, levels) {
  sample.push(prefix)

  if (!levels--)
    return

  var i
  for (var i = 0; i <= 2; i++)
    push(prefix + String.fromCharCode(i), levels)

  for (var i = 253; i <= 255; i++)
    push(prefix + String.fromCharCode(i), levels)
}

var sample = []
push('', 3)
sample.sort()

tape('utf8 sort should be preserved', function (t) {
  var result = sample.slice()
  t.deepEqual(result, sample)
  sample = sample.sort(function (a, b) {
    return compare(Buffer(a), Buffer(b))
  })
  t.deepEqual(result, sample)
  t.end()
})

tape('strings should round trip', function (t) {
  for (var i = 0; i < sample.length; i++) {
    var expected = sample[i]
    var result = bytewise.decode(bytewise.encode(expected))
    t.equal(result, expected)
  }
  t.end()
})

tape('nested strings should round trip', function (t) {
  var result = bytewise.decode(bytewise.encode(sample))
  t.deepEqual(result, sample)
  t.end()
})

tape('strings should sort correctly', function (t) {
  var result = sample.map(bytewise.encode)
    .sort(bytewise.compare)
    .map(bytewise.decode)

  t.deepEqual(result, sample)
  t.end()
})

tape('nested strings should sort correctly', function (t) {
  var nested = sample.map(function (s) {
      return [ s ]
    })

  var result = nested.map(bytewise.encode)
    .sort(bytewise.compare)
    .map(bytewise.decode)

  t.deepEqual(result, nested)
  t.end()
})
