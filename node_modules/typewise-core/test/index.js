var typewise = require('../');
var util = require('./util');
var tape = require('tape');

tape('simple', function (t) {
  var sample = util.getSample()
  var shuffled = util.shuffle(sample.slice())
  t.ok(typewise.equal(sample, shuffled.sort(typewise.compare)))
  t.deepEqual(sample, shuffled.sort(typewise.compare))

  var sample = util.getArraySample(2)
  var shuffled = util.shuffle(sample.slice())
  t.ok(typewise.equal(sample, shuffled.sort(typewise.compare)))
  t.deepEqual(sample, shuffled.sort(typewise.compare))
  t.end()
})
