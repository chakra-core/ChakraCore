var bytewise = require('../')
var tape = require('tape')
var util = require('typewise-core/test/util')

var sample, shuffled

function eq(t, a, b) {
  t.equal(a.length, b.length)
  a.forEach(function (_, i) {
    var y = b[i]
    var _a = bytewise.encode(a[i]).toString('hex')
    var _b = bytewise.encode(b[i]).toString('hex')
    
    t.equal(_a, _b)

    if (_a != _b) {
      console.log('not equal:', a[i])
      console.log('expected :', b[i])
    }
  })
}

tape('equal', function (t) {
  sample = util.getSample()
  shuffled = util.shuffle(sample.slice())

  eq(t, sample,
    shuffled
      .map(bytewise.encode)
      .sort(bytewise.compare)
      .map(bytewise.decode)
  )

  sample = util.getArraySample(2)
  shuffled = util.shuffle(sample.slice())
  eq(t, sample,
    shuffled.map(bytewise.encode).sort(bytewise.compare).map(bytewise.decode)
  )
  sample = util.shuffle(sample.slice())
  t.end()
})

var hash = {
  start: true,
  hash: sample,
  nested: {
    list: [sample]
  },
  end: {}
}

tape('simple equal', function (t) {
  eq(t, sample, bytewise.decode(bytewise.encode(sample)))
  t.end()
})

tape('encoded buffer toStrings to hex by default', function (t) {
  var encoded = bytewise.encode(sample)
  t.equal('' + encoded, encoded.toString('hex'))
  t.equal(Buffer(encoded).toString(), encoded.toString('utf8'))
  t.equal(encoded.toString('ascii'), Buffer(encoded.toString('hex'), 'hex').toString('ascii'))
  t.end()
})
