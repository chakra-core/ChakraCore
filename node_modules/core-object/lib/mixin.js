'use strict';

function arrayUnion(a, b) {
  let map = {};
  let i = 0;
  for (; i < a.length; i++) {
    map[a[i]] = true;
  }
  for (i = 0; i < b.length; i++) {
    map[b[i]] = true;
  }
  return Object.keys(map);
}

module.exports = function mixin(target, parent) {
  arrayUnion(Object.keys(target), Object.keys(parent)).forEach(function(key) {
    const targetValue = target[key];
    const parentValue = parent[key];
    
    // If both the object and the parent define the same key as a function
    if (typeof targetValue === 'function' && typeof parentValue === 'function') {
      if (targetValue.toString().indexOf('_super') > -1) {
        target[key] = function mixinSuperWrapper() {
          this._super = parent;
          return targetValue.apply(this, arguments);
        };
      }
    } else if (!(key in target)) {
      target[key] = parentValue;
    }
  });
  return target;
}