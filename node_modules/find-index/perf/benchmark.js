var cases = {};

if (typeof Array.prototype.findIndex === 'function') {
  cases['native Array.prototype.findIndex'] = function (array, predicate, context) {
    return array.findIndex(predicate, context);
  };
}

cases['findIndex'] = require('../findIndex');
cases['findIndex ponyfill'] = require('../ponyfill');
cases['findLastIndex'] = require('../findLastIndex');

cases['npm lodash.findindex'] = require('lodash.findindex');
cases['npm array-findindex'] = require('array-findindex');

// test setup
var testNumValues = 1000;

var testFindValue = 345;

// Populate the base array
var testArray = [];
for (var i = 0; i < testNumValues; i++) {
  testArray[i] = {id: i};
}

var testArrayReverse = testArray.slice(0).reverse();

var testContext = {id: testFindValue};

function testPredicate(item) {
  return item.id === testFindValue;
}

function testPredicateWithContext(item) {
  return item.id === this.id;
}

Object.keys(cases).forEach(function(caseName) {
  var findIndex = cases[caseName];
  // spec tests
  console.assert(findIndex(testArray, testPredicate) === testFindValue);
  console.assert(findIndex(testArray, testPredicateWithContext, testContext) === testFindValue);
  // perf test
  console.time(caseName);
  for (var i = 0; i < 100000; i++) {
    // test both ways to compensate for biased test value
    findIndex(testArray, testPredicate);
    findIndex(testArray, testPredicateWithContext, testContext);
    findIndex(testArrayReverse, testPredicate);
    findIndex(testArrayReverse, testPredicateWithContext, testContext);
  };
  console.timeEnd(caseName);
});
