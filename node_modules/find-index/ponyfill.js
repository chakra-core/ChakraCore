// Use the native ES6 `Array.findIndex` if available.
if (typeof Array.prototype.findIndex === 'function') {
  module.exports = function (array, predicate, self) {
    return array.findIndex(predicate, self);
  }
} else {
  module.exports = require('./findIndex');
}
