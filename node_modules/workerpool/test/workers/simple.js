// a simple worker
var workerpool = require('../../index');

function add(a, b) {
  return a + b;
}

function multiply(a, b) {
  return a * b;
}

function timeout(delay) {
  return new Promise(function (resolve, reject) {
    setTimeout(function () {
      resolve('done');
    }, delay)
  });
}

// create a worker and register some functions
workerpool.worker({
  add: add,
  multiply: multiply,
  timeout: timeout
});
