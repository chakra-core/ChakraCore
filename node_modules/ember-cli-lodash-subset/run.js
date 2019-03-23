var assert = require('assert');

console.time('lodash');
var lodash = require('./index');
console.timeEnd('lodash');

assert(typeof lodash.clone === 'function', 'clone is not a function');
assert(typeof lodash.cloneDeep === 'function', ' cloneDeep is not a function');
assert(typeof lodash.compact === 'function', ' compact is not a function');
assert(typeof lodash.debounce === 'function', ' debounce is not a function');
assert(typeof lodash.defaults === 'function', ' defaults is not a function');
assert(typeof lodash.defaultsDeep === 'function', ' defaultsDeep is not a function');
assert(typeof lodash.filter === 'function', ' filter is not a function');
assert(typeof lodash.find === 'function', ' find is not a function');
assert(typeof lodash.forEach === 'function', 'forEach is not a function');
assert(typeof lodash.forOwn === 'function', 'forOwn is not a function');
assert(typeof lodash.includes === 'function', ' includes is not a function');
assert(typeof lodash.intersection === 'function', ' intersection is not a function');
assert(typeof lodash.isEqual === 'function', ' isEqual is not a function');
assert(typeof lodash.isNull === 'function', ' isNull is not a function');
assert(typeof lodash.map === 'function', ' map is not a function');
assert(typeof lodash.merge === 'function', 'merge is not a function');
assert(typeof lodash.omitBy === 'function', ' omitBy is not a function');
assert(typeof lodash.reject === 'function', ' reject is not a function');
assert(typeof lodash.remove === 'function', ' remove is not a function');
assert(typeof lodash.union === 'function', 'union is not a function');
assert(typeof lodash.uniq === 'function', 'uniq is not a function');
assert(typeof lodash.uniqBy === 'function', ' uniqBy is not a function');
assert(typeof lodash.values === 'function', ' values is not a function');
assert(typeof lodash.zipObject === 'function', ' zipObject is not a function');

console.log('assertions passed');
console.log('on a fast machine (MBP SSD etc), the time above should be less than 10ms');
