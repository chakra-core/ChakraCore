'use strict';

const assignProperties = require('./lib/assign-properties');
const mixin = require('./lib/mixin');
const deprecation = require('./lib/deprecation');

class CoreObject {
  constructor() {
    this.init.apply(this, arguments);
  }

  init(options) {
    if (options) {
      for (let key in options) {
        this[key] = options[key];
      }
    }
  }

  static extend(options) {
    class Class extends this { }

    if (options) {
      if (shouldCallSuper(options.init)) {

        if (hasArgs(options.init)) {
          deprecation(
            'Overriding init without calling this._super is deprecated. ' +
            'Please call this._super(), addon: `' + options.name + '`'
          );
          options.init = forceSuperWithoutApply(options.init);
        } else {

          // this._super.init && is to make sure that the deprecation message
          // works for people who are writing addons supporting before 2.6.
          deprecation(
            'Overriding init without calling this._super is deprecated. ' +
            'Please call `this._super.init && this._super.init.apply(this, arguments);` addon: `' + options.name + '`'
          );
          options.init = forceSuper(options.init);
        }
      }
      assignProperties(Class.prototype, options);
    }

    return Class;
  }

  static mixin(target, mixinObj) {
    return mixin(target, mixinObj);
  }
}

function hasArgs(fn) {
  // Takes arguments, assume disruptive override
  return /^function *\( *[^ )]/.test(fn);
}

/* global define:true module:true window: true */
if (typeof define === 'function' && define['amd'])      { define(function() { return CoreObject; }); }
if (typeof module !== 'undefined' && module['exports']) { module['exports'] = CoreObject; }
if (typeof window !== 'undefined')                      { window['CoreObject'] = CoreObject; }

function shouldCallSuper(fn) {
  // No function, no problem
  if (!fn) { return false; }

  // Calls super already, good to go
  if (/this\._super\(/.test(fn)) { return false; }
  if (/this\._super\./.test(fn)) { return false; }

  return true;
}

function forceSuper(fn) {
  return function() {
    this._super.apply(this, arguments);
    fn.apply(this, arguments);
  }
}

function forceSuperWithoutApply(fn) {
  return function() {
    this._super.call(this);
    fn.apply(this, arguments);
  }
}
