'use strict';

const deprecation = require('./deprecation');

function setPrototypeOf(obj, proto) {
  Object.setPrototypeOf ? Object.setPrototypeOf(obj, proto) : obj.__proto__ = proto;
}

function getPrototypeOf(obj, proto) {
  return Object.getPrototypeOf ? Object.getPrototypeOf(obj) : obj.__proto__;
}

function giveMethodSuper(superclass, name, fn) {
  return function superWrapper() {
    let superFn;

    if (superclass[name]) {
      superFn = function() {
        return superclass[name].apply(this, arguments);
      }
    } else {
      superFn = function() {};
    }

    // #yolo
    setPrototypeOf(superFn, superclass);
    superFn.apply = Function.prototype.apply;
    superFn.call = Function.prototype.call;
    superFn.bind = Function.prototype.bind;
    superFn[name] = superclass[name] || function rootSuper() {};

    const previous = this._super;
    this._super = superFn;
    const ret = fn.apply(this, arguments);
    this._super = previous;
    return ret;
  };
}

const sourceAvailable = (function() {
  return this;
}).toString().indexOf('return this') > -1;

let hasSuper;
if (sourceAvailable) {
  hasSuper = function(fn) {
    if (fn.__hasSuper === undefined) {
     return fn.__hasSuper = fn.toString().indexOf('_super') > -1;
    } else {
     return fn.__hasSuper;
    }
  }
} else {
  hasSuper = function(target, fn) {
    return true;
  }
}

module.exports = function assignProperties(target, options) {
  let value;

  for (let key in options) {
    value = options[key];

    if (typeof value === 'function' && hasSuper(value)) {
      target[key] = giveMethodSuper(getPrototypeOf(target), key, value);
    } else {
      target[key] = value;
    }
  }
};
